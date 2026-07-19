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

UGSNPCComponent::UGSNPCComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	CurrentNPCState = ENPCState::None;
	bHasActiveOrder = false;
	AssignedTargetSpot = nullptr;
	MoneyItemClass = nullptr;
	bRequireCookedState = true;

	MinFoodWaitTime = 180.0f;
	MaxFoodWaitTime = 240.0f;
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
		World->GetTimerManager().ClearTimer(ArrivalCheckTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UGSNPCComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGSNPCComponent, CurrentNPCState);
	DOREPLIFETIME(UGSNPCComponent, ActiveOrder);
	DOREPLIFETIME(UGSNPCComponent, bHasActiveOrder);
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
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		MoveToCurrentSpot();
	}
}

void UGSNPCComponent::MoveToCurrentSpot()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArrivalCheckTimerHandle);

		if (!AssignedTargetSpot)
		{
			return;
		}

		APawn* PawnOwner = Cast<APawn>(GetOwner());
		if (!PawnOwner)
		{
			return;
		}

		AAIController* AIC = Cast<AAIController>(PawnOwner->GetController());
		if (!AIC)
		{
			World->GetTimerManager().SetTimerForNextTick(this, &UGSNPCComponent::MoveToCurrentSpot);
			return;
		}

		AIC->MoveToActor(AssignedTargetSpot, 50.0f);

		World->GetTimerManager().SetTimer(ArrivalCheckTimerHandle, this, &UGSNPCComponent::CheckArrival, 0.2f, true);
	}
}

void UGSNPCComponent::CheckArrival()
{
	if (!AssignedTargetSpot || !GetOwner())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ArrivalCheckTimerHandle);
		}
		return;
	}

	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), AssignedTargetSpot->GetActorLocation());
	if (Distance < 100.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ArrivalCheckTimerHandle);
		}
		HandleArrival();
	}
}

void UGSNPCComponent::HandleArrival()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (CurrentNPCState == ENPCState::Entering)
	{
		if (UWorld* World = GetWorld())
		{
			if (AGSGameState* GSGameState = Cast<AGSGameState>(World->GetGameState()))
			{
				if (AGSNPCManager* NPCManager = GSGameState->GetNPCManager())
				{
					int32 TableIdx = NPCManager->TableSpots.Find(AssignedTargetSpot);
					if (TableIdx != INDEX_NONE)
					{
						SetNPCState(ENPCState::Ordering);
						ChooseRandomOrder();
						SetNPCState(ENPCState::WaitingForFood);
					}
					else
					{
						SetNPCState(ENPCState::WaitingInQueue);
					}
				}
			}
		}
	}
	else if (CurrentNPCState == ENPCState::Leaving)
	{
		GetOwner()->Destroy();
	}
}
