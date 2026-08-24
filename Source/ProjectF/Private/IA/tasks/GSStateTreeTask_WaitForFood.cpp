#include "IA/tasks/GSStateTreeTask_WaitForFood.h"
#include "StateTreeExecutionContext.h"
#include "Components/GSNPCComponent.h"
#include "Components/GSNPCComponentAlphaTest.h"

EStateTreeRunStatus FGSStateTreeTask_WaitForFood::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AActor* Actor = InstanceData.Actor)
	{
		if (UGSNPCComponent* NPCComp = Actor->FindComponentByClass<UGSNPCComponent>())
		{
			NPCComp->SetNPCState(ENPCState::WaitingForFood);
			return EStateTreeRunStatus::Running;
		}
		else if (UGSNPCComponentAlphaTest* AlphaNPCComp = Actor->FindComponentByClass<UGSNPCComponentAlphaTest>())
		{
			AlphaNPCComp->SetNPCState(ENPCState::WaitingForFood);
			return EStateTreeRunStatus::Running;
		}
	}
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FGSStateTreeTask_WaitForFood::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AActor* Actor = InstanceData.Actor)
	{
		if (UGSNPCComponent* NPCComp = Actor->FindComponentByClass<UGSNPCComponent>())
		{
			if (NPCComp->CurrentNPCState != ENPCState::WaitingForFood)
			{
				return (NPCComp->CurrentNPCState == ENPCState::Eating) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
			}
			return EStateTreeRunStatus::Running;
		}
		else if (UGSNPCComponentAlphaTest* AlphaNPCComp = Actor->FindComponentByClass<UGSNPCComponentAlphaTest>())
		{
			if (AlphaNPCComp->CurrentNPCState != ENPCState::WaitingForFood)
			{
				return (AlphaNPCComp->CurrentNPCState == ENPCState::Eating) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
			}
			return EStateTreeRunStatus::Running;
		}
	}
	return EStateTreeRunStatus::Failed;
}
