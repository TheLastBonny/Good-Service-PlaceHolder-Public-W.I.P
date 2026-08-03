#include "IA/tasks/GSStateTreeTask_ReleaseSmartObject.h"
#include "StateTreeExecutionContext.h"
#include "SmartObjectSubsystem.h"
#include "Components/GSNPCComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

EStateTreeRunStatus FGSStateTreeTask_ReleaseSmartObject::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AActor* Actor = InstanceData.Actor;
	if (!Actor)
	{
		return EStateTreeRunStatus::Failed;
	}

	UGSNPCComponent* NPCComp = Actor->FindComponentByClass<UGSNPCComponent>();
	AActor* SpotToRelease = InstanceData.TargetSpot ? InstanceData.TargetSpot.Get() : (NPCComp ? NPCComp->AssignedTargetSpot.Get() : nullptr);

	if (NPCComp)
	{
		NPCComp->SetAssignedTargetSpot(nullptr);
	}

	FString SpotName = SpotToRelease ? SpotToRelease->GetName() : TEXT("Assigned Spot");
	UE_LOG(LogTemp, Warning, TEXT("[RELEASE_DEBUG] NPC '%s' released SmartObject Spot '%s'"),
		*Actor->GetName(), *SpotName);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow,
			FString::Printf(TEXT("[RELEASE OK] %s -> Released Spot: %s"), *Actor->GetName(), *SpotName));
	}

	return EStateTreeRunStatus::Succeeded;
}
