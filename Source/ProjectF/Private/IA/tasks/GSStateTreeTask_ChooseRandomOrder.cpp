#include "IA/tasks/GSStateTreeTask_ChooseRandomOrder.h"
#include "StateTreeExecutionContext.h"
#include "Components/GSNPCComponent.h"

const UStruct* FGSStateTreeTask_ChooseRandomOrder::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FGSStateTreeTask_ChooseRandomOrder::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AActor* Actor = InstanceData.Actor)
	{
		if (UGSNPCComponent* NPCComp = Actor->FindComponentByClass<UGSNPCComponent>())
		{
			NPCComp->ChooseRandomOrder();
			return EStateTreeRunStatus::Succeeded;
		}
	}
	return EStateTreeRunStatus::Failed;
}
