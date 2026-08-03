#include "Components/GSNPCComponentAlphaTest.h"
#include "Net/UnrealNetwork.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "SmartObjectSubsystem.h"
#include "SmartObjectComponent.h"
#include "SmartObjectRequestTypes.h"
#include "Items/GSItem.h"
#include "Items/GSMoneyItem.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AbilitySystemComponent.h"
#include "Attributes/GSCookingAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "Characters/GSPlayerInterface.h"

UGSNPCComponentAlphaTest::UGSNPCComponentAlphaTest()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	CurrentNPCState = ENPCState::None;
	CurrentStateDescription = TEXT("None");
	ChairSmartObjectTag = FGameplayTag::RequestGameplayTag(FName("SmartObject.Seat.Dining"));
	ExitSmartObjectTag = FGameplayTag::RequestGameplayTag(FName("SmartObject.Exit"));
	SearchRadius = 5000.0f;
	bAutoRunStateFlow = true;
	bRequireCookedState = true;
	FoodWaitTime = 45.0f;
	EatingTime = 8.0f;
	bShowDebugLogs = false;
	bHasActiveOrder = false;
	AssignedTargetSpot = nullptr;
	TargetLocation = FVector::ZeroVector;
}

void UGSNPCComponentAlphaTest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGSNPCComponentAlphaTest, CurrentNPCState);
	DOREPLIFETIME(UGSNPCComponentAlphaTest, ActiveOrder);
	DOREPLIFETIME(UGSNPCComponentAlphaTest, bHasActiveOrder);
}

void UGSNPCComponentAlphaTest::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner())
	{
		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
		{
			RootPrim->OnComponentBeginOverlap.AddDynamic(this, &UGSNPCComponentAlphaTest::OnCapsuleOverlap);
			RootPrim->OnComponentHit.AddDynamic(this, &UGSNPCComponentAlphaTest::OnCapsuleHit);
		}

		if (GetOwner()->HasAuthority())
		{
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] Component initialized on %s. AutoRunStateFlow: %d"),
					*GetOwner()->GetName(), bAutoRunStateFlow);
			}

			if (bAutoRunStateFlow)
			{
				SetNPCState(ENPCState::Entering);
				ClaimAndMoveToSmartObject(ChairSmartObjectTag);
			}
		}
	}
}

void UGSNPCComponentAlphaTest::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FoodWaitTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EatingTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(MovementPollTimerHandle);
	}

	ReleaseCurrentSmartObject();
	RemoveOrderWidget();

	Super::EndPlay(EndPlayReason);
}

void UGSNPCComponentAlphaTest::SetNPCState(ENPCState NewState)
{
	if (CurrentNPCState == NewState) return;

	ENPCState OldState = CurrentNPCState;
	CurrentNPCState = NewState;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FoodWaitTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EatingTimerHandle);
	}

	static const UEnum* EnumPtr = StaticEnum<ENPCState>();
	CurrentStateDescription = EnumPtr ? EnumPtr->GetNameStringByValue(static_cast<int64>(NewState)) : TEXT("Unknown");

	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s State Changed: %s -> %s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"),
			EnumPtr ? *EnumPtr->GetNameStringByValue(static_cast<int64>(OldState)) : TEXT("?"),
			*CurrentStateDescription);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Magenta,
				FString::Printf(TEXT("[ALPHA NPC %s] State: %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("NPC"), *CurrentStateDescription));
		}
	}

	OnNPCStateChanged.Broadcast(NewState);
}

void UGSNPCComponentAlphaTest::OnRep_CurrentNPCState()
{
	static const UEnum* EnumPtr = StaticEnum<ENPCState>();
	CurrentStateDescription = EnumPtr ? EnumPtr->GetNameStringByValue(static_cast<int64>(CurrentNPCState)) : TEXT("Unknown");
	OnNPCStateChanged.Broadcast(CurrentNPCState);
}

bool UGSNPCComponentAlphaTest::ClaimAndMoveToSmartObject(FGameplayTag TargetTag)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return false;

	UWorld* World = OwnerActor->GetWorld();
	if (!World) return false;

	USmartObjectSubsystem* SOSubsystem = USmartObjectSubsystem::GetCurrent(World);
	if (!SOSubsystem)
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Error, TEXT("[ALPHA_NPC] SmartObjectSubsystem is NULL on %s!"), *OwnerActor->GetName());
		}
		return false;
	}

	FSmartObjectRequestFilter Filter;
	if (TargetTag.IsValid())
	{
		Filter.ActivityRequirements = FGameplayTagQuery::MakeQuery_MatchTag(TargetTag);
	}

	FVector Origin = OwnerActor->GetActorLocation();
	float Radius = SearchRadius > 0.0f ? SearchRadius : 5000.0f;
	FBox SearchBounds = FBox::BuildAABB(Origin, FVector(Radius, Radius, Radius));

	FSmartObjectRequest Request(SearchBounds, Filter);
	TArray<FSmartObjectRequestResult> Results;
	SOSubsystem->FindSmartObjects(Request, Results, OwnerActor);

	FString TagStr = TargetTag.IsValid() ? TargetTag.ToString() : TEXT("NONE");

	for (const FSmartObjectRequestResult& Result : Results)
	{
		FSmartObjectClaimHandle ClaimHandle = SOSubsystem->MarkSlotAsClaimed(Result.SlotHandle, ESmartObjectClaimPriority::Normal);
		if (ClaimHandle.IsValid())
		{
			FTransform SlotTransform;
			if (SOSubsystem->GetSlotTransform(ClaimHandle, SlotTransform))
			{
				CurrentClaimHandle = ClaimHandle;
				TargetLocation = SlotTransform.GetLocation();

				USmartObjectComponent* SOComp = SOSubsystem->GetSmartObjectComponent(ClaimHandle);
				AssignedTargetSpot = SOComp ? SOComp->GetOwner() : nullptr;

				AActor* TargetActor = AssignedTargetSpot.Get();
				FString SpotName = TargetActor ? TargetActor->GetName() : TEXT("Unknown");
				if (bShowDebugLogs)
				{
					UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s Claimed Spot '%s' (Tag: %s) at %s"),
						*OwnerActor->GetName(), *SpotName, *TagStr, *TargetLocation.ToString());
				}

				APawn* OwnerPawn = Cast<APawn>(OwnerActor);
				if (OwnerPawn)
				{
					if (!OwnerPawn->GetController())
					{
						OwnerPawn->SpawnDefaultController();
						if (bShowDebugLogs)
						{
							UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] Spawned default AIController for Pawn %s"), *OwnerActor->GetName());
						}
					}

					AAIController* AICon = Cast<AAIController>(OwnerPawn->GetController());
					if (AICon)
					{
						float StartDist = FVector::Dist(OwnerActor->GetActorLocation(), TargetLocation);
						EPathFollowingRequestResult::Type MoveResult = EPathFollowingRequestResult::Failed;

						if (AssignedTargetSpot.IsValid())
						{
							FAIMoveRequest MoveReq(AssignedTargetSpot.Get());
							MoveReq.SetAcceptanceRadius(80.0f);
							MoveReq.SetUsePathfinding(true);
							MoveReq.SetProjectGoalLocation(true);
							FPathFollowingRequestResult MoveReqResult = AICon->MoveTo(MoveReq);
							MoveResult = MoveReqResult.Code;
						}
						else
						{
							FAIMoveRequest MoveReq(TargetLocation);
							MoveReq.SetAcceptanceRadius(80.0f);
							MoveReq.SetUsePathfinding(true);
							MoveReq.SetProjectGoalLocation(true);
							FPathFollowingRequestResult MoveReqResult = AICon->MoveTo(MoveReq);
							MoveResult = MoveReqResult.Code;
						}

						if (bShowDebugLogs)
						{
							UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] AIController MoveTo Result: %d (0=AlreadyAtGoal, 1=RequestSuccessful, 2=Failed) | StartDist: %.1fu | %s -> Target: %s"),
								static_cast<int32>(MoveResult), StartDist, *OwnerActor->GetName(), *TargetLocation.ToString());
						}
					}
					else if (bShowDebugLogs)
					{
						UE_LOG(LogTemp, Error, TEXT("[ALPHA_NPC] FAILED to get AAIController for Pawn %s! Check Auto Possess AI on Pawn Blueprint."), *OwnerActor->GetName());
					}
				}

				World->GetTimerManager().SetTimer(MovementPollTimerHandle, this, &UGSNPCComponentAlphaTest::PollMovementToLocation, 0.033f, true);
				return true;
			}
		}
	}

	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Error, TEXT("[ALPHA_NPC] FAILED to claim SmartObject for tag: %s on %s"), *TagStr, *OwnerActor->GetName());
	}
	return false;
}

void UGSNPCComponentAlphaTest::ReleaseCurrentSmartObject()
{
	if (CurrentClaimHandle.IsValid() && GetOwner() && GetOwner()->GetWorld())
	{
		if (USmartObjectSubsystem* SOSubsystem = USmartObjectSubsystem::GetCurrent(GetOwner()->GetWorld()))
		{
			SOSubsystem->Release(CurrentClaimHandle);
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] Released SmartObject Handle for %s"), *GetOwner()->GetName());
			}
		}
		CurrentClaimHandle.Invalidate();
	}
	AssignedTargetSpot = nullptr;
}

void UGSNPCComponentAlphaTest::PollMovementToLocation()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	AAIController* AICon = OwnerPawn ? Cast<AAIController>(OwnerPawn->GetController()) : nullptr;

	FVector CurrentLoc = OwnerActor->GetActorLocation();
	FVector Dir = (TargetLocation - CurrentLoc);
	Dir.Z = 0.0f;
	float Dist = Dir.Size();

	bool bIsTouchingTarget = false;
	if (AssignedTargetSpot.IsValid())
	{
		AActor* TargetActor = AssignedTargetSpot.Get();
		if (TargetActor && OwnerActor->IsOverlappingActor(TargetActor))
		{
			bIsTouchingTarget = true;
		}
	}

	bool bArrived = bIsTouchingTarget || (Dist <= 160.0f);

	if (bArrived)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(MovementPollTimerHandle);
		}

		if (APawn* Pawn = Cast<APawn>(OwnerActor))
		{
			if (Pawn->Implements<UGSPlayerInterface>())
			{
				IGSPlayerInterface::Execute_RequestMove(Pawn, FVector2D::ZeroVector);
			}
		}

		if (AssignedTargetSpot.IsValid())
		{
			FVector LookDir = AssignedTargetSpot.Get()->GetActorLocation() - CurrentLoc;
			LookDir.Z = 0.0f;
			if (!LookDir.IsNearlyZero())
			{
				OwnerActor->SetActorRotation(LookDir.Rotation());
			}
		}

		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s ARRIVED near TargetLocation! Dist: %.1f"), *OwnerActor->GetName(), Dist);
		}

		if (CurrentNPCState == ENPCState::Entering)
		{
			SetNPCState(ENPCState::Ordering);
			ChooseRandomOrder();
			StartWaitingForFoodTimer();
			SetNPCState(ENPCState::WaitingForFood);
		}
		else if (CurrentNPCState == ENPCState::Leaving)
		{
			ReleaseCurrentSmartObject();
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s Reached Exit. Destroying Actor."), *OwnerActor->GetName());
			}
			OwnerActor->Destroy();
		}
	}
	else
	{
		FVector NavDir = Dir;
		if (AICon && AICon->GetPathFollowingComponent())
		{
			FVector PathDir = AICon->GetPathFollowingComponent()->GetCurrentDirection();
			PathDir.Z = 0.0f;
			if (!PathDir.IsNearlyZero())
			{
				NavDir = PathDir;
			}
		}
		NavDir.Normalize();

		APawn* Pawn = Cast<APawn>(OwnerActor);
		if (Pawn)
		{
			FRotator TargetRot = NavDir.Rotation();
			TargetRot.Pitch = 0.0f;
			TargetRot.Roll = 0.0f;

			if (AController* PawnCon = Pawn->GetController())
			{
				PawnCon->SetControlRotation(TargetRot);
			}

			Pawn->SetActorRotation(TargetRot);
			Pawn->AddMovementInput(NavDir, 1.0f);

			if (Pawn->Implements<UGSPlayerInterface>())
			{
				IGSPlayerInterface::Execute_RequestMove(Pawn, FVector2D(0.0f, 1.0f));
			}
		}
	}
}

void UGSNPCComponentAlphaTest::ChooseRandomOrder()
{
	if (!LevelMenu || LevelMenu->AvailableRecipes.Num() == 0)
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Error, TEXT("[ALPHA_NPC] Cannot choose order: LevelMenu is NULL or Empty on %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));
		}
		return;
	}

	int32 RandomIndex = FMath::RandRange(0, LevelMenu->AvailableRecipes.Num() - 1);
	ActiveOrder = LevelMenu->AvailableRecipes[RandomIndex];
	bHasActiveOrder = true;

	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s Chose Order: %s (FoodTag: %s)"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"),
			*ActiveOrder.FoodName.ToString(),
			*ActiveOrder.FoodTag.ToString());
	}

	CreateOrUpdateOrderWidget();
	OnOrderChosen.Broadcast(ActiveOrder);
}

bool UGSNPCComponentAlphaTest::CheckIfItemMatchesOrder(AGSItem* Item) const
{
	if (!Item || !bHasActiveOrder) return false;

	UAbilitySystemComponent* ItemASC = Item->GetAbilitySystemComponent();
	if (!ItemASC) return false;

	if (ActiveOrder.FoodTag.IsValid() && !ItemASC->HasMatchingGameplayTag(ActiveOrder.FoodTag))
	{
		return false;
	}

	return true;
}

bool UGSNPCComponentAlphaTest::DeliverItem(AGSItem* Item)
{
	if (!Item || CurrentNPCState != ENPCState::WaitingForFood) return false;

	if (CheckIfItemMatchesOrder(Item))
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s ACCEPTED food delivery: %s!"),
				GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"), *Item->GetName());
		}

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(FoodWaitTimerHandle);
		}

		Item->Destroy();
		RemoveOrderWidget();
		OnOrderFulfilled.Broadcast(true);

		SetNPCState(ENPCState::Eating);
		StartEatingTimer();
		return true;
	}

	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s REJECTED food delivery: %s (Does not match order)"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"), *Item->GetName());
	}
	return false;
}

void UGSNPCComponentAlphaTest::StartWaitingForFoodTimer()
{
	if (!GetWorld()) return;
	float WaitTime = FoodWaitTime > 0.0f ? FoodWaitTime : 45.0f;
	GetWorld()->GetTimerManager().SetTimer(FoodWaitTimerHandle, this, &UGSNPCComponentAlphaTest::HandleFoodWaitTimeout, WaitTime, false);

	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("[ALPHA_NPC] Started Food Wait Timer (%.1fs) for %s"), WaitTime, GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));
	}
}

void UGSNPCComponentAlphaTest::HandleFoodWaitTimeout()
{
	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s Food Wait TIMED OUT! Customer leaving unsatisfied."), GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));
	}

	OnOrderFulfilled.Broadcast(false);
	RemoveOrderWidget();

	SetNPCState(ENPCState::Leaving);
	ReleaseCurrentSmartObject();
	ClaimAndMoveToSmartObject(ExitSmartObjectTag);
}

void UGSNPCComponentAlphaTest::StartEatingTimer()
{
	if (!GetWorld()) return;
	float TimeToEat = EatingTime > 0.0f ? EatingTime : 8.0f;
	GetWorld()->GetTimerManager().SetTimer(EatingTimerHandle, this, &UGSNPCComponentAlphaTest::HandleEatingFinished, TimeToEat, false);

	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("[ALPHA_NPC] %s Started eating for %.1fs"), GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"), TimeToEat);
	}
}

void UGSNPCComponentAlphaTest::HandleEatingFinished()
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority() && MoneyItemClass)
	{
		FVector ForwardOffset = OwnerActor->GetActorForwardVector() * 75.0f;
		FVector SpawnLoc = OwnerActor->GetActorLocation() + ForwardOffset + FVector(0, 0, 15.0f);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AGSMoneyItem>(MoneyItemClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);

		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s Finished eating! Spawned money item 75u forward on table."), *OwnerActor->GetName());
		}
	}

	SetNPCState(ENPCState::Leaving);
	ReleaseCurrentSmartObject();
	ClaimAndMoveToSmartObject(ExitSmartObjectTag);
}

void UGSNPCComponentAlphaTest::CreateOrUpdateOrderWidget()
{
	if (!OrderWidgetClass || !GetOwner()) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC)
	{
		PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	if (!ActiveOrderWidgetInstance && PC && PC->IsLocalController())
	{
		ActiveOrderWidgetInstance = CreateWidget<UUserWidget>(PC, OrderWidgetClass);
		if (ActiveOrderWidgetInstance)
		{
			ActiveOrderWidgetInstance->AddToViewport();
		}
	}

	OnOrderWidgetUpdated.Broadcast(ActiveOrderWidgetInstance, ActiveOrder);
}

void UGSNPCComponentAlphaTest::RemoveOrderWidget()
{
	if (ActiveOrderWidgetInstance)
	{
		ActiveOrderWidgetInstance->RemoveFromParent();
		ActiveOrderWidgetInstance = nullptr;
	}
}

void UGSNPCComponentAlphaTest::OnCapsuleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (CurrentNPCState == ENPCState::WaitingForFood && OtherActor)
	{
		if (AGSItem* Item = Cast<AGSItem>(OtherActor))
		{
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] Capsule Overlapped by Item: %s! Checking order..."), *Item->GetName());
			}
			DeliverItem(Item);
		}
	}
}

void UGSNPCComponentAlphaTest::OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (CurrentNPCState == ENPCState::WaitingForFood && OtherActor)
	{
		if (AGSItem* Item = Cast<AGSItem>(OtherActor))
		{
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] Capsule Hit by Item: %s! Checking order..."), *Item->GetName());
			}
			DeliverItem(Item);
		}
	}
}
