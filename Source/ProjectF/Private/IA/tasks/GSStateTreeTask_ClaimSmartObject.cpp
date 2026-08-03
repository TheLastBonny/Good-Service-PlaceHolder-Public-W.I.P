#include "IA/tasks/GSStateTreeTask_ClaimSmartObject.h"
#include "StateTreeExecutionContext.h"
#include "SmartObjectSubsystem.h"
#include "SmartObjectComponent.h"
#include "SmartObjectRequestTypes.h"
#include "SmartObjectTypes.h"
#include "SmartObjectRuntime.h"
#include "Components/GSNPCComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

EStateTreeRunStatus FGSStateTreeTask_ClaimSmartObject::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.bSuccess = false;
	InstanceData.AssignedTargetSpot = nullptr;
	InstanceData.TargetLocation = FVector::ZeroVector;

	AActor* Actor = InstanceData.Actor;
	if (!Actor)
	{
		UE_LOG(LogTemp, Error, TEXT("[CLAIM_DEBUG] FGSStateTreeTask_ClaimSmartObject FAILED: Actor Context is NULL!"));
		return EStateTreeRunStatus::Failed;
	}

	UWorld* World = Actor->GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[CLAIM_DEBUG] FGSStateTreeTask_ClaimSmartObject FAILED on %s: World is NULL!"), *Actor->GetName());
		return EStateTreeRunStatus::Failed;
	}

	USmartObjectSubsystem* SOSubsystem = USmartObjectSubsystem::GetCurrent(World);
	if (!SOSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[CLAIM_DEBUG] FGSStateTreeTask_ClaimSmartObject FAILED on %s: SmartObjectSubsystem is NULL!"), *Actor->GetName());
		return EStateTreeRunStatus::Failed;
	}

	FSmartObjectRequestFilter Filter;
	if (InstanceData.SmartObjectTag.IsValid())
	{
		Filter.ActivityRequirements = FGameplayTagQuery::MakeQuery_MatchTag(InstanceData.SmartObjectTag);
	}

	FVector Origin = Actor->GetActorLocation();
	float Radius = InstanceData.SearchRadius > 0.0f ? InstanceData.SearchRadius : 5000.0f;
	FVector BoxExtent(Radius, Radius, Radius);
	FBox SearchBounds = FBox::BuildAABB(Origin, BoxExtent);

	FSmartObjectRequest Request(SearchBounds, Filter);
	TArray<FSmartObjectRequestResult> Results;
	SOSubsystem->FindSmartObjects(Request, Results, Actor);

	FString TagNameStr = InstanceData.SmartObjectTag.IsValid() ? InstanceData.SmartObjectTag.ToString() : TEXT("NONE");

	if (Results.Num() == 0)
	{
		FSmartObjectRequest UnfilteredRequest(SearchBounds, FSmartObjectRequestFilter());
		TArray<FSmartObjectRequestResult> UnfilteredResults;
		SOSubsystem->FindSmartObjects(UnfilteredRequest, UnfilteredResults, Actor);

		UE_LOG(LogTemp, Warning, TEXT("[CLAIM_DIAGNOSTIC] Filtered search for tag '%s' returned 0 results. Total registered SmartObjects in range %.0fu (any tag): %d"),
			*TagNameStr, Radius, UnfilteredResults.Num());

		for (int32 i = 0; i < UnfilteredResults.Num(); ++i)
		{
			const FSmartObjectRequestResult& UnfilteredRes = UnfilteredResults[i];
			USmartObjectComponent* SOComp = SOSubsystem->GetSmartObjectComponentByRequestResult(UnfilteredRes);
			AActor* SOOwner = SOComp ? SOComp->GetOwner() : nullptr;
			UE_LOG(LogTemp, Warning, TEXT("[CLAIM_DIAGNOSTIC] Candidate #%d: Owner='%s'"),
				i, SOOwner ? *SOOwner->GetName() : TEXT("NULL"));
		}
	}

	for (const FSmartObjectRequestResult& Result : Results)
	{
		FSmartObjectClaimHandle ClaimHandle = SOSubsystem->MarkSlotAsClaimed(Result.SlotHandle, ESmartObjectClaimPriority::Normal);
		if (ClaimHandle.IsValid())
		{
			FTransform SlotTransform;
			if (SOSubsystem->GetSlotTransform(ClaimHandle, SlotTransform))
			{
				InstanceData.TargetLocation = SlotTransform.GetLocation();
				InstanceData.ClaimHandle = ClaimHandle;

				USmartObjectComponent* SOComp = SOSubsystem->GetSmartObjectComponent(ClaimHandle);
				if (SOComp)
				{
					InstanceData.AssignedTargetSpot = SOComp->GetOwner();
				}

				if (UGSNPCComponent* NPCComp = Actor->FindComponentByClass<UGSNPCComponent>())
				{
					NPCComp->SetAssignedTargetSpot(InstanceData.AssignedTargetSpot);
				}

				InstanceData.bSuccess = true;

				FString SpotName = InstanceData.AssignedTargetSpot ? InstanceData.AssignedTargetSpot->GetName() : TEXT("UNKNOWN_SPOT_ACTOR");
				FString LocStr = InstanceData.TargetLocation.ToString();

				UE_LOG(LogTemp, Warning, TEXT("[CLAIM_DEBUG] SUCCESS! NPC '%s' claimed SmartObject Spot '%s' | Tag: %s | Location: %s | bSuccess: TRUE"),
					*Actor->GetName(), *SpotName, *TagNameStr, *LocStr);

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan,
						FString::Printf(TEXT("[CLAIM OK] %s -> Spot: %s | Pos: %s"), *Actor->GetName(), *SpotName, *LocStr));
				}

				return InstanceData.bReleaseOnExit ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
			}
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[CLAIM_DEBUG] FAILED! NPC '%s' could NOT claim any SmartObject matching Tag: %s (Found %d candidates in range %.0fu)"),
		*Actor->GetName(), *TagNameStr, Results.Num(), Radius);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red,
			FString::Printf(TEXT("[CLAIM FAILED] %s -> No available SmartObject for tag: %s"), *Actor->GetName(), *TagNameStr));
	}

	return EStateTreeRunStatus::Failed;
}

void FGSStateTreeTask_ClaimSmartObject::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.bReleaseOnExit && InstanceData.ClaimHandle.IsValid())
	{
		AActor* Actor = InstanceData.Actor;
		if (Actor && Actor->GetWorld())
		{
			if (USmartObjectSubsystem* SOSubsystem = USmartObjectSubsystem::GetCurrent(Actor->GetWorld()))
			{
				SOSubsystem->Release(InstanceData.ClaimHandle);
				UE_LOG(LogTemp, Warning, TEXT("[CLAIM_DEBUG] bReleaseOnExit active: Released SmartObject handle for NPC '%s'"),
					*Actor->GetName());

				if (UGSNPCComponent* NPCComp = Actor->FindComponentByClass<UGSNPCComponent>())
				{
					NPCComp->SetAssignedTargetSpot(nullptr);
				}
			}
		}
		InstanceData.ClaimHandle.Invalidate();
	}
}
