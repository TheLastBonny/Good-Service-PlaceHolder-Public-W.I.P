#include "Core/GSNPCManager.h"
#include "Core/GSGameState.h"
#include "Components/GSNPCComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

AGSNPCManager::AGSNPCManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SpawnInterval = 8.0f;
	MaxActiveNPCs = 5;
	NPCPawnClass = nullptr;
	ExitSpot = nullptr;
}

void AGSNPCManager::BeginPlay()
{
	Super::BeginPlay();

	// Initialize tracking arrays to match spot arrays sizes
	QueueOccupants.Init(nullptr, QueueSpots.Num());
	TableOccupants.Init(nullptr, TableSpots.Num());

	// Auto-register with the GameState
	if (UWorld* World = GetWorld())
	{
		if (AGSGameState* GSGameState = Cast<AGSGameState>(World->GetGameState()))
		{
			GSGameState->RegisterNPCManager(this);
		}
	}
}

void AGSNPCManager::StartSpawning()
{
	if (HasAuthority() && NPCPawnClass)
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AGSNPCManager::HandlePeriodicSpawn, SpawnInterval, true);
	}
}

void AGSNPCManager::StopSpawning()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

void AGSNPCManager::HandlePeriodicSpawn()
{
	if (HasAuthority())
	{
		if (ActiveNPCs.Num() < MaxActiveNPCs)
		{
			SpawnNPC();
		}
	}
}

APawn* AGSNPCManager::SpawnNPC()
{
	if (!HasAuthority() || !NPCPawnClass || SpawnPoints.Num() == 0 || QueueSpots.Num() == 0)
	{
		return nullptr;
	}

	// Find the first empty spot in the queue line
	int32 FreeQueueIdx = INDEX_NONE;
	for (int32 i = 0; i < QueueOccupants.Num(); ++i)
	{
		if (QueueOccupants[i] == nullptr)
		{
			FreeQueueIdx = i;
			break;
		}
	}

	// If the queue line is completely full, skip spawning
	if (FreeQueueIdx == INDEX_NONE)
	{
		return nullptr;
	}

	// Pick a random spawn point
	int32 RandomSpawnIdx = FMath::RandRange(0, SpawnPoints.Num() - 1);
	AActor* SpawnPoint = SpawnPoints[RandomSpawnIdx];
	if (!SpawnPoint)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector SpawnLoc = SpawnPoint->GetActorLocation();
	FRotator SpawnRot = SpawnPoint->GetActorRotation();

	if (APawn* NewNPC = GetWorld()->SpawnActor<APawn>(NPCPawnClass, SpawnLoc, SpawnRot, SpawnParams))
	{
		ActiveNPCs.Add(NewNPC);
		QueueOccupants[FreeQueueIdx] = NewNPC;

		if (UGSNPCComponent* NPCComp = Cast<UGSNPCComponent>(NewNPC->GetComponentByClass(UGSNPCComponent::StaticClass())))
		{
			NPCComp->LevelMenu = Cast<AGSGameState>(GetWorld()->GetGameState())->GetNPCManager() ? 
				// The game state tree or level menu asset is fetched via level state.
				// We let the NPC select from their assigned LevelMenu which is edited on the Blueprint.
				NPCComp->LevelMenu : nullptr;

			// Assign target spot in the queue
			NPCComp->AssignedTargetSpot = QueueSpots[FreeQueueIdx];
			NPCComp->SetNPCState(ENPCState::Entering);
		}

		return NewNPC;
	}

	return nullptr;
}

bool AGSNPCManager::AssignTableToNPC(APawn* NPC)
{
	if (!HasAuthority() || !NPC)
	{
		return false;
	}

	// Find first empty dining table
	int32 FreeTableIdx = INDEX_NONE;
	for (int32 j = 0; j < TableOccupants.Num(); ++j)
	{
		if (TableOccupants[j] == nullptr)
		{
			FreeTableIdx = j;
			break;
		}
	}

	if (FreeTableIdx == INDEX_NONE)
	{
		// No table available, NPC will have to leave directly
		return false;
	}

	// Remove from queue spot
	int32 QueueIdx = QueueOccupants.Find(NPC);
	if (QueueIdx != INDEX_NONE)
	{
		QueueOccupants[QueueIdx] = nullptr;
	}

	// Assign table spot
	TableOccupants[FreeTableIdx] = NPC;

	if (UGSNPCComponent* NPCComp = Cast<UGSNPCComponent>(NPC->GetComponentByClass(UGSNPCComponent::StaticClass())))
	{
		NPCComp->AssignedTargetSpot = TableSpots[FreeTableIdx];
		NPCComp->SetNPCState(ENPCState::Eating);
	}

	// Compress the queue since a hole was created
	ShiftQueue();
	return true;
}

void AGSNPCManager::SendNPCToExit(APawn* NPC)
{
	if (!HasAuthority() || !NPC)
	{
		return;
	}

	// Remove from queue spots if they were still waiting in line
	int32 QueueIdx = QueueOccupants.Find(NPC);
	if (QueueIdx != INDEX_NONE)
	{
		QueueOccupants[QueueIdx] = nullptr;
		ShiftQueue();
	}

	// Remove from table spots
	int32 TableIdx = TableOccupants.Find(NPC);
	if (TableIdx != INDEX_NONE)
	{
		TableOccupants[TableIdx] = nullptr;
	}

	// Assign ExitSpot as target
	if (UGSNPCComponent* NPCComp = Cast<UGSNPCComponent>(NPC->GetComponentByClass(UGSNPCComponent::StaticClass())))
	{
		NPCComp->AssignedTargetSpot = ExitSpot;
		NPCComp->SetNPCState(ENPCState::Leaving);
	}

	// Remove from tracking list (the NPC Actor will destroy itself once it reaches exit or via StateTree)
	ActiveNPCs.Remove(NPC);
}

void AGSNPCManager::ShiftQueue()
{
	if (!HasAuthority() || QueueSpots.Num() == 0)
	{
		return;
	}

	// Compact queue occupants array to remove null entries (gaps)
	TArray<TObjectPtr<APawn>> CompactedOccupants;
	for (APawn* NPC : QueueOccupants)
	{
		if (NPC != nullptr)
		{
			CompactedOccupants.Add(NPC);
		}
	}

	// Re-initialize QueueOccupants with nulls
	QueueOccupants.Init(nullptr, QueueSpots.Num());

	// Re-fill occupants from index 0 (front of the line)
	for (int32 i = 0; i < CompactedOccupants.Num() && i < QueueOccupants.Num(); ++i)
	{
		QueueOccupants[i] = CompactedOccupants[i];

		APawn* ShiftedNPC = QueueOccupants[i];
		if (ShiftedNPC)
		{
			if (UGSNPCComponent* NPCComp = Cast<UGSNPCComponent>(ShiftedNPC->GetComponentByClass(UGSNPCComponent::StaticClass())))
			{
				// Only re-route NPC if their queue spot index changed
				if (NPCComp->AssignedTargetSpot != QueueSpots[i])
				{
					NPCComp->AssignedTargetSpot = QueueSpots[i];
					NPCComp->SetNPCState(ENPCState::Entering);
				}
			}
		}
	}
}
