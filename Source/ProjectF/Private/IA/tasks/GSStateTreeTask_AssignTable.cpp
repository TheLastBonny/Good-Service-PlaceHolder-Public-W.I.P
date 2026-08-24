#include "IA/tasks/GSStateTreeTask_AssignTable.h"
#include "StateTreeExecutionContext.h"
#include "Core/GSNPCManager.h"
#include "Core/GSGameState.h"
#include "Components/GSNPCComponent.h"
#include "Engine/World.h"

const UStruct* FGSStateTreeTask_AssignTable::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FGSStateTreeTask_AssignTable::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.bSuccess = false;
	InstanceData.AssignedTargetSpot = nullptr;

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
						bool bAssigned = NPCManager->AssignTableToNPC(Pawn);
						InstanceData.bSuccess = bAssigned;

						if (bAssigned)
						{
							if (UGSNPCComponent* NPCComp = Pawn->FindComponentByClass<UGSNPCComponent>())
							{
								InstanceData.AssignedTargetSpot = NPCComp->AssignedTargetSpot;
							}
						}

						return bAssigned ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
					}
				}
			}
		}
	}
	return EStateTreeRunStatus::Failed;
}
