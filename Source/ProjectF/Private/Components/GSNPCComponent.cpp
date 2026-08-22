#include "Components/GSNPCComponent.h"
#include "Items/GSItem.h"
#include "Items/GSMoneyItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Core/GSGameState.h"
#include "Core/GSNPCManager.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "AIController.h"
#include "Components/GSBillboardWidgetComponent.h"
#include "GameFramework/GameStateBase.h"
#include "UI/GSNPCOrderWidget.h"
#include "UI/GSFloatingWidgetBase.h"

UGSNPCComponent::UGSNPCComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	CurrentNPCState = ENPCState::None;
	bHasActiveOrder = false;
	AssignedTargetSpot = nullptr;
	MoneyItemClass = nullptr;
	bRequireCookedState = true;

	TotalFoodWaitTime = 0.0f;
	FoodWaitStartTime = 0.0f;
	MinFoodWaitTime = 30.0f;
	MaxFoodWaitTime = 45.0f;
	MinEatingTime = 8.0f;
	MaxEatingTime = 12.0f;
	OrderTimeoutPenalty = 50;
	PendingMoneyValue = 0.0f;
}

void UGSNPCComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGSNPCComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FoodWaitTimerHandle);
		World->GetTimerManager().ClearTimer(EatingTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UGSNPCComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGSNPCComponent, CurrentNPCState);
	DOREPLIFETIME(UGSNPCComponent, ActiveOrder);
	DOREPLIFETIME(UGSNPCComponent, bHasActiveOrder);
	DOREPLIFETIME(UGSNPCComponent, TotalFoodWaitTime);
	DOREPLIFETIME(UGSNPCComponent, FoodWaitStartTime);
}

void UGSNPCComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentNPCState == ENPCState::WaitingForFood && TotalFoodWaitTime > 0.0f && ActiveOrderWidgetInstance && GetWorld())
	{
		float CurrentServerTime = 0.0f;
		if (AGameStateBase* GS = GetWorld()->GetGameState())
		{
			CurrentServerTime = GS->GetServerWorldTimeSeconds();
		}
		else
		{
			CurrentServerTime = GetWorld()->GetTimeSeconds();
		}

		float Elapsed = CurrentServerTime - FoodWaitStartTime;
		float Percent = FMath::Clamp(1.0f - (Elapsed / TotalFoodWaitTime), 0.0f, 1.0f);

		if (UGSNPCOrderWidget* OrderWidget = Cast<UGSNPCOrderWidget>(ActiveOrderWidgetInstance))
		{
			OrderWidget->SetPatiencePercent(Percent);
		}
		else if (UGSFloatingWidgetBase* BaseWidget = Cast<UGSFloatingWidgetBase>(ActiveOrderWidgetInstance))
		{
			BaseWidget->SetProgress(Percent);
		}
	}
}

void UGSNPCComponent::SetNPCState(ENPCState NewState)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (CurrentNPCState != NewState)
		{
			ENPCState OldState = CurrentNPCState;
			CurrentNPCState = NewState;
			
			OnNPCStateChanged.Broadcast(CurrentNPCState);
			HandleStateChangedServer(OldState, CurrentNPCState);
		}
	}
}

void UGSNPCComponent::OnRep_CurrentNPCState()
{
	OnNPCStateChanged.Broadcast(CurrentNPCState);

	if (CurrentNPCState == ENPCState::Ordering || CurrentNPCState == ENPCState::WaitingForFood)
	{
		if (bHasActiveOrder && ActiveOrder.FoodTag.IsValid())
		{
			CreateOrUpdateOrderWidget();
		}
	}
	else if (CurrentNPCState == ENPCState::Eating || CurrentNPCState == ENPCState::Leaving)
	{
		RemoveOrderWidget();
	}
}

void UGSNPCComponent::OnRep_ActiveOrder()
{
	if (bHasActiveOrder && ActiveOrder.FoodTag.IsValid())
	{
		CreateOrUpdateOrderWidget();
	}
	else
	{
		RemoveOrderWidget();
	}
}

void UGSNPCComponent::HandleStateChangedServer(ENPCState OldState, ENPCState NewState)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FoodWaitTimerHandle);
		World->GetTimerManager().ClearTimer(EatingTimerHandle);

		if (NewState == ENPCState::WaitingForFood)
		{
			float WaitTime = FMath::RandRange(MinFoodWaitTime, MaxFoodWaitTime);
			TotalFoodWaitTime = WaitTime;

			float CurrentServerTime = 0.0f;
			if (AGameStateBase* GS = World->GetGameState())
			{
				CurrentServerTime = GS->GetServerWorldTimeSeconds();
			}
			else
			{
				CurrentServerTime = World->GetTimeSeconds();
			}
			FoodWaitStartTime = CurrentServerTime;

			World->GetTimerManager().SetTimer(FoodWaitTimerHandle, this, &UGSNPCComponent::HandleFoodTimeout, WaitTime, false);
		}
		else if (NewState == ENPCState::Eating)
		{
			float EatTime = FMath::RandRange(MinEatingTime, MaxEatingTime);
			World->GetTimerManager().SetTimer(EatingTimerHandle, this, &UGSNPCComponent::HandleEatingFinished, EatTime, false);
		}
	}
}

void UGSNPCComponent::HandleFoodTimeout()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	RemoveOrderWidget();
	OnOrderFulfilled.Broadcast(false);

	if (UWorld* World = GetWorld())
	{
		if (AGSGameState* GSGameState = Cast<AGSGameState>(World->GetGameState()))
		{
			GSGameState->AddMoneyDirectly(-static_cast<float>(OrderTimeoutPenalty));
		}
	}

	bHasActiveOrder = false;
	ActiveOrder = FGSFoodRecipeDetails();

	SetNPCState(ENPCState::Leaving);

	bool bSentToExit = false;
	if (UWorld* World = GetWorld())
	{
		if (AGSGameState* GSGameState = Cast<AGSGameState>(World->GetGameState()))
		{
			if (AGSNPCManager* NPCManager = GSGameState->GetNPCManager())
			{
				NPCManager->SendNPCToExit(Cast<APawn>(GetOwner()));
				bSentToExit = true;
			}
		}
	}

	if (!bSentToExit && GetOwner())
	{
		GetOwner()->SetLifeSpan(3.0f);
	}
}

void UGSNPCComponent::HandleEatingFinished()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (MoneyItemClass && PendingMoneyValue > 0.0f)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		FVector SpawnLoc = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
		FRotator SpawnRot = GetOwner()->GetActorRotation();

		if (AGSMoneyItem* SpawnedMoney = GetWorld()->SpawnActor<AGSMoneyItem>(MoneyItemClass, SpawnLoc, SpawnRot, SpawnParams))
		{
			SpawnedMoney->MoneyValue = PendingMoneyValue;
		}
	}

	PendingMoneyValue = 0.0f;
	SetNPCState(ENPCState::Leaving);

	if (UWorld* World = GetWorld())
	{
		if (AGSGameState* GSGameState = Cast<AGSGameState>(World->GetGameState()))
		{
			if (AGSNPCManager* NPCManager = GSGameState->GetNPCManager())
			{
				NPCManager->SendNPCToExit(Cast<APawn>(GetOwner()));
			}
		}
	}
}

void UGSNPCComponent::ChooseRandomOrder()
{
	if (LevelMenu)
	{
		ActiveOrder = LevelMenu->GetRandomRecipe();
		if (ActiveOrder.FoodTag.IsValid())
		{
			bHasActiveOrder = true;
			CreateOrUpdateOrderWidget();
			OnOrderChosen.Broadcast(ActiveOrder);
		}
	}
}

bool UGSNPCComponent::CheckIfItemMatchesOrder(AGSItem* Item) const
{
	if (!Item || !bHasActiveOrder)
	{
		return false;
	}

	UAbilitySystemComponent* ItemASC = Item->GetAbilitySystemComponent();
	if (ItemASC)
	{
		bool bMatchesFood = ItemASC->HasMatchingGameplayTag(ActiveOrder.FoodTag);
		
		if (bRequireCookedState)
		{
			bool bIsCooked = ItemASC->HasMatchingGameplayTag(GSGameplayTags::State_Cooked);
			bool bIsBurned = ItemASC->HasMatchingGameplayTag(GSGameplayTags::State_Burned);

			return bMatchesFood && bIsCooked && !bIsBurned;
		}

		return bMatchesFood;
	}

	return Item->ItemTags.HasTagExact(ActiveOrder.FoodTag);
}

bool UGSNPCComponent::DeliverItem(AGSItem* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!Item || !bHasActiveOrder)
	{
		return false;
	}

	if (CheckIfItemMatchesOrder(Item))
	{
		PendingMoneyValue = ActiveOrder.BasePrice;

		bHasActiveOrder = false;
		ActiveOrder = FGSFoodRecipeDetails();

		RemoveOrderWidget();
		OnOrderFulfilled.Broadcast(true);

		Item->Destroy();

		SetNPCState(ENPCState::Eating);

		return true;
	}

	return false;
}

void UGSNPCComponent::SetAssignedTargetSpot(AActor* NewSpot)
{
	AssignedTargetSpot = NewSpot;
}

void UGSNPCComponent::CreateOrUpdateOrderWidget()
{
	if (!OrderWidgetClass || !GetOwner()) return;

	AActor* OwnerActor = GetOwner();
	if (!OrderWidgetComponent && OwnerActor)
	{
		OrderWidgetComponent = OwnerActor->FindComponentByClass<UGSBillboardWidgetComponent>();
		if (!OrderWidgetComponent)
		{
			OrderWidgetComponent = NewObject<UGSBillboardWidgetComponent>(OwnerActor, TEXT("NPCOrderWidgetComp"));
			if (OrderWidgetComponent)
			{
				OrderWidgetComponent->RegisterComponent();
				USceneComponent* RootComp = OwnerActor->GetRootComponent();
				if (RootComp)
				{
					OrderWidgetComponent->AttachToComponent(RootComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				}
				OrderWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
			}
		}
	}

	if (OrderWidgetComponent)
	{
		OrderWidgetComponent->SetWidgetClass(OrderWidgetClass);
		OrderWidgetComponent->SetVisibility(true);

		ActiveOrderWidgetInstance = OrderWidgetComponent->GetUserWidgetObject();
		if (UGSNPCOrderWidget* OrderWidget = Cast<UGSNPCOrderWidget>(ActiveOrderWidgetInstance))
		{
			OrderWidget->SetOrderDetails(ActiveOrder);
		}
		else if (UGSFloatingWidgetBase* BaseWidget = Cast<UGSFloatingWidgetBase>(ActiveOrderWidgetInstance))
		{
			BaseWidget->SetWidgetData(ActiveOrder.FoodIcon, ActiveOrder.FoodName, FText::GetEmpty(), 1.0f);
		}
	}
}

void UGSNPCComponent::RemoveOrderWidget()
{
	if (OrderWidgetComponent)
	{
		OrderWidgetComponent->SetVisibility(false);
	}
	ActiveOrderWidgetInstance = nullptr;
}
