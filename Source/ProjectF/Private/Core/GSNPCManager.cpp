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
	bAlwaysRelevant = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	SpawnInterval = 8.0f;
	MaxActiveNPCs = 5;
	NPCPawnClass = nullptr;
	ExitSpot = nullptr;
	bShowDebugLogs = false;
}

void AGSNPCManager::BeginPlay()
{
	Super::BeginPlay();


	QueueOccupants.Init(nullptr, QueueSpots.Num());
	TableOccupants.Init(nullptr, TableSpots.Num());


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
		if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] StartSpawning: Timer started with Interval %fs"), SpawnInterval);
			UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, Msg);
		}
	}
	else if (bShowDebugLogs)
	{
		FString Msg = FString::Printf(TEXT("[GSNPCManager] StartSpawning FAILED: HasAuthority=%d, NPCPawnClass=%s"), HasAuthority(), NPCPawnClass ? *NPCPawnClass->GetName() : TEXT("NULL"));
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Msg);
	}
}

void AGSNPCManager::StopSpawning()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("[GSNPCManager] StopSpawning: Timer cleared"));
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, TEXT("[GSNPCManager] Spawning stopped"));
		}
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
		else if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] HandlePeriodicSpawn: Skipped (Active NPCs %d >= Max %d)"), ActiveNPCs.Num(), MaxActiveNPCs);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, Msg);
		}
	}
}

APawn* AGSNPCManager::SpawnNPC()
{
	if (!HasAuthority())
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] SpawnNPC FAILED: No Authority (Client trying to spawn?)"));
		}
		return nullptr;
	}

	if (!NPCPawnClass)
	{
		if (bShowDebugLogs)
		{
			FString Msg = TEXT("[GSNPCManager] SpawnNPC FAILED: NPCPawnClass is NULL! Make sure to assign it in Details panel.");
			UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
		}
		return nullptr;
	}

	if (SpawnPoints.Num() == 0)
	{
		if (bShowDebugLogs)
		{
			FString Msg = TEXT("[GSNPCManager] SpawnNPC FAILED: SpawnPoints array is empty! Make sure to place spawn points and link them.");
			UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
		}
		return nullptr;
	}

	if (QueueSpots.Num() == 0)
	{
		if (bShowDebugLogs)
		{
			FString Msg = TEXT("[GSNPCManager] SpawnNPC FAILED: QueueSpots array is empty! Make sure to place queue spots and link them.");
			UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
		}
		return nullptr;
	}


	int32 FreeQueueIdx = INDEX_NONE;
	for (int32 i = 0; i < QueueOccupants.Num(); ++i)
	{
		if (QueueOccupants[i] == nullptr)
		{
			FreeQueueIdx = i;
			break;
		}
	}


	if (FreeQueueIdx == INDEX_NONE)
	{
		if (bShowDebugLogs)
		{
			FString Msg = TEXT("[GSNPCManager] SpawnNPC FAILED: All Queue spots are full!");
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, Msg);
		}
		return nullptr;
	}


	int32 RandomSpawnIdx = FMath::RandRange(0, SpawnPoints.Num() - 1);
	AActor* SpawnPoint = SpawnPoints[RandomSpawnIdx];
	if (!SpawnPoint)
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Error, TEXT("[GSNPCManager] SpawnNPC FAILED: Chosen SpawnPoint actor is NULL"));
		}
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
				NPCComp->LevelMenu : nullptr;

			NPCComp->SetNPCState(ENPCState::Entering);
			NPCComp->SetAssignedTargetSpot(QueueSpots[FreeQueueIdx]);
		}
		else if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] SpawnNPC WARNING: Spawned Pawn %s does not have a GSNPCComponent!"), *NewNPC->GetName());
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Msg);
		}

		if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] SpawnNPC SUCCESS: Spawned %s and assigned to Queue spot %d"), *NewNPC->GetName(), FreeQueueIdx);
			UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, Msg);
		}

		return NewNPC;
	}

	if (bShowDebugLogs)
	{
		FString Msg = TEXT("[GSNPCManager] SpawnNPC FAILED: SpawnActor returned NULL (Check collision settings or Pawn class)");
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
	}

	return nullptr;
}

bool AGSNPCManager::AssignTableToNPC(APawn* NPC)
{
	if (!HasAuthority() || !NPC)
	{
		return false;
	}


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

		return false;
	}


	int32 QueueIdx = QueueOccupants.Find(NPC);
	if (QueueIdx != INDEX_NONE)
	{
		QueueOccupants[QueueIdx] = nullptr;
	}


	TableOccupants[FreeTableIdx] = NPC;

	if (UGSNPCComponent* NPCComp = Cast<UGSNPCComponent>(NPC->GetComponentByClass(UGSNPCComponent::StaticClass())))
	{
		NPCComp->SetNPCState(ENPCState::Entering);
		NPCComp->SetAssignedTargetSpot(TableSpots[FreeTableIdx]);
	}


	ShiftQueue();
	return true;
}

void AGSNPCManager::SendNPCToExit(APawn* NPC)
{
	if (!HasAuthority() || !NPC)
	{
		return;
	}


	int32 QueueIdx = QueueOccupants.Find(NPC);
	if (QueueIdx != INDEX_NONE)
	{
		QueueOccupants[QueueIdx] = nullptr;
		ShiftQueue();
	}


	int32 TableIdx = TableOccupants.Find(NPC);
	if (TableIdx != INDEX_NONE)
	{
		TableOccupants[TableIdx] = nullptr;
	}


	if (UGSNPCComponent* NPCComp = Cast<UGSNPCComponent>(NPC->GetComponentByClass(UGSNPCComponent::StaticClass())))
	{
		NPCComp->SetNPCState(ENPCState::Leaving);
		NPCComp->SetAssignedTargetSpot(ExitSpot);
	}


	ActiveNPCs.Remove(NPC);
}

void AGSNPCManager::ShiftQueue()
{
	if (!HasAuthority() || QueueSpots.Num() == 0)
	{
		return;
	}


	TArray<TObjectPtr<APawn>> CompactedOccupants;
	for (APawn* NPC : QueueOccupants)
	{
		if (NPC != nullptr)
		{
			CompactedOccupants.Add(NPC);
		}
	}


	QueueOccupants.Init(nullptr, QueueSpots.Num());


	for (int32 i = 0; i < CompactedOccupants.Num() && i < QueueOccupants.Num(); ++i)
	{
		QueueOccupants[i] = CompactedOccupants[i];

		APawn* ShiftedNPC = QueueOccupants[i];
		if (ShiftedNPC)
		{
			if (UGSNPCComponent* NPCComp = Cast<UGSNPCComponent>(ShiftedNPC->GetComponentByClass(UGSNPCComponent::StaticClass())))
			{
				if (NPCComp->AssignedTargetSpot != QueueSpots[i])
				{
					NPCComp->SetAssignedTargetSpot(QueueSpots[i]);
				}
			}
		}
	}
}
