#include "IA/tasks/GSStateTreeTask_SendToExit.h"
#include "StateTreeExecutionContext.h"
#include "Core/GSNPCManager.h"
#include "Core/GSGameState.h"
#include "Engine/World.h"

const UStruct* FGSStateTreeTask_SendToExit::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FGSStateTreeTask_SendToExit::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AActor* Actor = InstanceData.Actor)
	{
		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			if (UWorld* World = Actor->GetWorld())
			{
				if (AGSGameState* GameState = Cast<AGSGameState>(World->GetGameState()))
				{
					if (AGSNPCManager* NPCManager = GameState->GetNPCManager())
					{
						NPCManager->SendNPCToExit(Pawn);
						return EStateTreeRunStatus::Succeeded;
					}
				}
			}
		}
	}
	return EStateTreeRunStatus::Failed;
}
