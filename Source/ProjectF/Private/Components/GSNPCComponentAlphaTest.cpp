#include "Components/GSNPCComponentAlphaTest.h"
#include "Net/UnrealNetwork.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "SmartObjectSubsystem.h"
#include "SmartObjectComponent.h"
#include "SmartObjectRequestTypes.h"
#include "Items/GSItem.h"
#include "Items/GSMoneyItem.h"
#include "Blueprint/UserWidget.h"
#include "Components/GSBillboardWidgetComponent.h"
#include "UI/GSNPCOrderWidget.h"
#include "UI/GSFloatingWidgetBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AbilitySystemComponent.h"
#include "Attributes/GSCookingAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "Characters/GSPlayerInterface.h"

UGSNPCComponentAlphaTest::UGSNPCComponentAlphaTest()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	CurrentNPCState = ENPCState::None;
	CurrentStateDescription = TEXT("None");
	ChairSmartObjectTag = FGameplayTag::RequestGameplayTag(FName("SmartObject.Seat.Dining"));
	ExitSmartObjectTag = FGameplayTag::RequestGameplayTag(FName("SmartObject.Exit"));
	SearchRadius = 5000.0f;
	bAutoRunStateFlow = true;
	bRequireCookedState = true;
	TotalFoodWaitTime = 0.0f;
	FoodWaitStartTime = 0.0f;
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
	DOREPLIFETIME(UGSNPCComponentAlphaTest, TotalFoodWaitTime);
	DOREPLIFETIME(UGSNPCComponentAlphaTest, FoodWaitStartTime);
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
		if (NewState != ENPCState::WaitingForFood)
		{
			GetWorld()->GetTimerManager().ClearTimer(FoodWaitTimerHandle);
		}
		if (NewState != ENPCState::Eating)
		{
			GetWorld()->GetTimerManager().ClearTimer(EatingTimerHandle);
		}
	}

	if (NewState == ENPCState::WaitingForFood)
	{
		StartWaitingForFoodTimer();
	}
	else if (NewState == ENPCState::Eating)
	{
		StartEatingTimer();
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

void UGSNPCComponentAlphaTest::OnRep_ActiveOrder()
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
				TargetLocation = SlotTransform.GetLocation();

				USmartObjectComponent* SOComp = SOSubsystem->GetSmartObjectComponent(ClaimHandle);
				AssignedTargetSpot = SOComp ? SOComp->GetOwner() : nullptr;

				// Release exit claim handle immediately so other NPCs leaving can also use the exit spot
				if (TargetTag.IsValid() && TargetTag == ExitSmartObjectTag)
				{
					SOSubsystem->Release(ClaimHandle);
					CurrentClaimHandle.Invalidate();
				}
				else
				{
					CurrentClaimHandle = ClaimHandle;
				}

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
						FVector NavTargetLocation = TargetLocation;
						if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
						{
							FNavLocation ProjectedNavLoc;
							if (NavSys->ProjectPointToNavigation(TargetLocation, ProjectedNavLoc, FVector(150.0f, 150.0f, 100.0f)))
							{
								NavTargetLocation = ProjectedNavLoc.Location;
							}
						}

						CurrentNavTargetLocation = NavTargetLocation;
						float StartDist = FVector::Dist(OwnerActor->GetActorLocation(), NavTargetLocation);
						EPathFollowingRequestResult::Type MoveResult = EPathFollowingRequestResult::Failed;

						FAIMoveRequest MoveReq(NavTargetLocation);
						MoveReq.SetAcceptanceRadius(120.0f);
						MoveReq.SetUsePathfinding(true);
						MoveReq.SetProjectGoalLocation(true);
						MoveReq.SetAllowPartialPath(true);
						FPathFollowingRequestResult MoveReqResult = AICon->MoveTo(MoveReq);
						MoveResult = MoveReqResult.Code;
						bHasIssuedMoveRequest = true;

						FString ControllerClassStr = AICon->GetClass()->GetName();
						FString PathCompClassStr = AICon->GetPathFollowingComponent() ? AICon->GetPathFollowingComponent()->GetClass()->GetName() : TEXT("NONE");

						int32 PathPointsCount = 0;
						bool bIsPartialPath = false;
						if (AICon->GetPathFollowingComponent() && AICon->GetPathFollowingComponent()->GetPath().IsValid())
						{
							FNavPathSharedPtr Path = AICon->GetPathFollowingComponent()->GetPath();
							PathPointsCount = Path->GetPathPoints().Num();
							bIsPartialPath = Path->IsPartial();

							if (bShowDebugLogs)
							{
								const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();
								for (int32 idx = 0; idx < PathPoints.Num() - 1; idx++)
								{
									DrawDebugLine(World, PathPoints[idx].Location + FVector(0, 0, 30), PathPoints[idx + 1].Location + FVector(0, 0, 30), FColor::Cyan, false, 5.0f, 0, 3.0f);
									DrawDebugPoint(World, PathPoints[idx].Location + FVector(0, 0, 30), 10.0f, FColor::Yellow, false, 5.0f);
								}
								if (PathPoints.Num() > 0)
								{
									DrawDebugPoint(World, PathPoints.Last().Location + FVector(0, 0, 30), 14.0f, FColor::Green, false, 5.0f);
								}
							}
						}

						if (bShowDebugLogs)
						{
							UE_LOG(LogTemp, Warning, TEXT("[NPC_DEBUG_CLAIM] %s Claimed SmartObject!"), *OwnerActor->GetName());
							UE_LOG(LogTemp, Warning, TEXT("  -> Controller Class: %s"), *ControllerClassStr);
							UE_LOG(LogTemp, Warning, TEXT("  -> PathFollowing Class: %s"), *PathCompClassStr);
							UE_LOG(LogTemp, Warning, TEXT("  -> RawTarget: %s | NavProjectedTarget: %s | StartDist: %.1fu"), *TargetLocation.ToString(), *NavTargetLocation.ToString(), StartDist);
							UE_LOG(LogTemp, Warning, TEXT("  -> AICon MoveTo Result: %d (0=AlreadyAtGoal, 1=Successful, 2=Failed) | PathPoints: %d | IsPartial: %d"),
								static_cast<int32>(MoveResult), PathPointsCount, bIsPartialPath ? 1 : 0);
						}

						if (MoveResult == EPathFollowingRequestResult::Failed && World)
						{
							World->GetTimerManager().SetTimer(InitialMoveRetryTimerHandle, this, &UGSNPCComponentAlphaTest::RetryInitialMove, 0.1f, false);
						}
					}
					else
					{
						if (bShowDebugLogs)
						{
							UE_LOG(LogTemp, Error, TEXT("[NPC_DEBUG_CLAIM] FAILED to get AAIController for Pawn %s! Controller is NULL or not AAIController."), *OwnerActor->GetName());
						}
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

void UGSNPCComponentAlphaTest::RetryInitialMove()
{
	if (CurrentNPCState == ENPCState::Entering)
	{
		ClaimAndMoveToSmartObject(ChairSmartObjectTag);
	}
	else if (CurrentNPCState == ENPCState::Leaving)
	{
		ClaimAndMoveToSmartObject(ExitSmartObjectTag);
	}
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
	FVector DirToRaw = (TargetLocation - CurrentLoc);
	DirToRaw.Z = 0.0f;
	float DistToRaw = DirToRaw.Size();

	FVector DirToNav = (CurrentNavTargetLocation - CurrentLoc);
	DirToNav.Z = 0.0f;
	float DistToNav = DirToNav.Size();

	bool bIsTouchingTarget = false;
	if (AssignedTargetSpot.IsValid())
	{
		AActor* TargetActor = AssignedTargetSpot.Get();
		if (TargetActor && OwnerActor->IsOverlappingActor(TargetActor))
		{
			bIsTouchingTarget = true;
		}
	}

	bool bPathReachedGoal = false;
	bool bPathIsIdleAndClose = false;

	if (AICon && AICon->GetPathFollowingComponent())
	{
		UPathFollowingComponent* PathComp = AICon->GetPathFollowingComponent();
		bPathReachedGoal = PathComp->DidMoveReachGoal();
		EPathFollowingStatus::Type Status = PathComp->GetStatus();
		if (Status == EPathFollowingStatus::Idle && (DistToRaw <= 380.0f || DistToNav <= 150.0f) && bHasIssuedMoveRequest)
		{
			bPathIsIdleAndClose = true;
		}
	}

	bool bArrived = bIsTouchingTarget || (DistToRaw <= 200.0f) || (DistToNav <= 120.0f) || bPathReachedGoal || bPathIsIdleAndClose;

	if (bArrived)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(MovementPollTimerHandle);
			GetWorld()->GetTimerManager().ClearTimer(InitialMoveRetryTimerHandle);
		}

		if (APawn* Pawn = Cast<APawn>(OwnerActor))
		{
			if (AICon)
			{
				AICon->StopMovement();
			}
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
			UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s ARRIVED near TargetLocation! DistRaw: %.1f | DistNav: %.1f"), *OwnerActor->GetName(), DistToRaw, DistToNav);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green,
					FString::Printf(TEXT("[ALPHA NPC %s] ARRIVED AT SEAT! Dist: %.1fu"), *OwnerActor->GetName(), DistToRaw));
			}
		}

		if (CurrentNPCState == ENPCState::Entering)
		{
			SetNPCState(ENPCState::Ordering);
			ChooseRandomOrder();
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
		float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

		if (AICon && AICon->GetPathFollowingComponent())
		{
			if (AICon->GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Idle && DistToNav > 400.0f)
			{
				if (CurrentTime - LastMoveRetryTime >= 3.0f)
				{
					LastMoveRetryTime = CurrentTime;

					FAIMoveRequest MoveReq(CurrentNavTargetLocation);
					MoveReq.SetAcceptanceRadius(120.0f);
					MoveReq.SetUsePathfinding(true);
					MoveReq.SetProjectGoalLocation(true);
					MoveReq.SetAllowPartialPath(true);
					AICon->MoveTo(MoveReq);

					if (bShowDebugLogs)
					{
						UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] Retried MoveTo for %s (Stalled far away at DistNav: %.1fu)"), *OwnerActor->GetName(), DistToNav);
					}
				}
			}
		}

		if (bShowDebugLogs && (CurrentTime - LastLogTime >= 1.0f))
		{
			LastLogTime = CurrentTime;

			FString PathStatusStr = TEXT("NO_PATH_COMP");
			FVector PathDir = FVector::ZeroVector;
			if (AICon && AICon->GetPathFollowingComponent())
			{
				UPathFollowingComponent* PathComp = AICon->GetPathFollowingComponent();
				PathDir = PathComp->GetCurrentDirection();
				switch (PathComp->GetStatus())
				{
				case EPathFollowingStatus::Idle: PathStatusStr = TEXT("Idle"); break;
				case EPathFollowingStatus::Waiting: PathStatusStr = TEXT("Waiting"); break;
				case EPathFollowingStatus::Paused: PathStatusStr = TEXT("Paused"); break;
				case EPathFollowingStatus::Moving: PathStatusStr = TEXT("Moving"); break;
				}
			}

			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[NPC_DEBUG_POLL] '%s' | DistToNav: %.1fu | PathStatus: %s | Dir: %s"),
					*OwnerActor->GetName(), DistToNav, *PathStatusStr, *PathDir.ToString());
			}

			if (GEngine && bShowDebugLogs)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Magenta,
					FString::Printf(TEXT("[%s] DistNav: %.1fu | PathStatus: %s"), *OwnerActor->GetName(), DistToNav, *PathStatusStr));
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

void UGSNPCComponentAlphaTest::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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

void UGSNPCComponentAlphaTest::StartWaitingForFoodTimer()
{
	if (!GetWorld()) return;
	float WaitTime = FoodWaitTime > 0.0f ? FoodWaitTime : 45.0f;
	TotalFoodWaitTime = WaitTime;

	float CurrentServerTime = 0.0f;
	if (AGameStateBase* GS = GetWorld()->GetGameState())
	{
		CurrentServerTime = GS->GetServerWorldTimeSeconds();
	}
	else
	{
		CurrentServerTime = GetWorld()->GetTimeSeconds();
	}
	FoodWaitStartTime = CurrentServerTime;

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
	bool bMoved = ClaimAndMoveToSmartObject(ExitSmartObjectTag);

	if (!bMoved && GetOwner())
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ALPHA_NPC] %s could not find Exit SmartObject! Destroying actor fallback."), *GetOwner()->GetName());
		}
		GetOwner()->SetLifeSpan(3.0f);
	}
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

	OnOrderWidgetUpdated.Broadcast(ActiveOrderWidgetInstance, ActiveOrder);
}

void UGSNPCComponentAlphaTest::RemoveOrderWidget()
{
	if (OrderWidgetComponent)
	{
		OrderWidgetComponent->SetVisibility(false);
	}
	ActiveOrderWidgetInstance = nullptr;
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
