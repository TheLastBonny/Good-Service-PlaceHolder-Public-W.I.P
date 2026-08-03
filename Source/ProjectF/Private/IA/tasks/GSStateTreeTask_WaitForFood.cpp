#include "IA/tasks/GSStateTreeTask_WaitForFood.h"
#include "StateTreeExecutionContext.h"
#include "Components/GSNPCComponent.h"

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
	}
	return EStateTreeRunStatus::Failed;
}
