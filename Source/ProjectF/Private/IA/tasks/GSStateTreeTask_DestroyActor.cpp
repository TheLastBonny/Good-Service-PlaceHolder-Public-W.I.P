#include "IA/tasks/GSStateTreeTask_DestroyActor.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FGSStateTreeTask_DestroyActor::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AActor* Actor = InstanceData.Actor)
	{
		Actor->Destroy();
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Failed;
}
