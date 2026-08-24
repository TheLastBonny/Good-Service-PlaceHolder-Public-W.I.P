#include "IA/tasks/GSStateTreeTask_ControlNPCSpawner.h"
#include "StateTreeExecutionContext.h"
#include "Core/GSGameState.h"
#include "Core/GSNPCManager.h"
#include "Engine/World.h"

EStateTreeRunStatus FGSStateTreeTask_ControlNPCSpawner::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UWorld* World = InstanceData.Actor ? InstanceData.Actor->GetWorld() : Context.GetWorld();

	if (World)
	{
		if (AGSGameState* GSGameState = Cast<AGSGameState>(World->GetGameState()))
		{
			if (AGSNPCManager* NPCManager = GSGameState->GetNPCManager())
			{
				if (InstanceData.bResetWaveProgress)
				{
					NPCManager->ResetWaveProgress();
				}

				if (InstanceData.bStartSpawning)
				{
					NPCManager->StartSpawning();
					UE_LOG(LogTemp, Warning, TEXT("[StateTreeTask] Control NPC Spawner -> Started Spawning"));
				}
				else
				{
					NPCManager->StopSpawning();
					UE_LOG(LogTemp, Warning, TEXT("[StateTreeTask] Control NPC Spawner -> Stopped Spawning"));
				}
				return EStateTreeRunStatus::Succeeded;
			}
		}
	}

	return EStateTreeRunStatus::Failed;
}
