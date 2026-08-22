#include "Core/GSNPCManager.h"
#include "Core/GSGameState.h"
#include "Core/GSGameplayTags.h"
#include "Components/GSNPCComponent.h"
#include "Components/GSNPCComponentAlphaTest.h"
#include "Characters/GSPawn.h"
#include "DataAssets/UGSCharacterDataAsset.h"
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
	MaxNPCsPerWave = 10;
	TotalNPCsSpawnedInWave = 0;
	bAutoListenToGamePhase = true;
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
			if (bAutoListenToGamePhase)
			{
				GSGameState->OnPrimaryPhaseTagChanged.AddDynamic(this, &AGSNPCManager::HandleGamePhaseTagChanged);
				GSGameState->OnGamePhaseChanged.AddDynamic(this, &AGSNPCManager::HandleGamePhaseChanged);
			}
		}
	}
}

void AGSNPCManager::ResetWaveProgress()
{
	TotalNPCsSpawnedInWave = 0;
	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] ResetWaveProgress: TotalNPCsSpawnedInWave reset to 0"));
	}
}

void AGSNPCManager::HandleGamePhaseTagChanged(FGameplayTag NewPhaseTag)
{
	if (!HasAuthority()) return;

	if (NewPhaseTag == GSGameplayTags::GamePhase_Core_RoundInProgress)
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] PhaseTag -> RoundInProgress! Starting NPC Spawning."));
		}
		ResetWaveProgress();
		StartSpawning();
	}
	else if (NewPhaseTag == GSGameplayTags::GamePhase_Core_WaitingToStart || NewPhaseTag == GSGameplayTags::GamePhase_Core_RoundOver)
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] PhaseTag -> WaitingToStart/RoundOver! Stopping NPC Spawning."));
		}
		StopSpawning();
	}
}

void AGSNPCManager::HandleGamePhaseChanged(EGSGamePhase NewPhase)
{
	if (!HasAuthority()) return;

	if (NewPhase == EGSGamePhase::RoundInProgress)
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] PhaseEnum -> RoundInProgress! Starting NPC Spawning."));
		}
		ResetWaveProgress();
		StartSpawning();
	}
	else if (NewPhase == EGSGamePhase::WaitingToStart || NewPhase == EGSGamePhase::RoundOver)
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] PhaseEnum -> WaitingToStart/RoundOver! Stopping NPC Spawning."));
		}
		StopSpawning();
	}
}

void AGSNPCManager::StartSpawning()
{
	if (HasAuthority() && NPCPawnClass)
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AGSNPCManager::HandlePeriodicSpawn, SpawnInterval, true);
		if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] StartSpawning: Timer started with Interval %.1fs (Wave Max: %d)"), SpawnInterval, MaxNPCsPerWave);
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
		if (MaxNPCsPerWave > 0 && TotalNPCsSpawnedInWave >= MaxNPCsPerWave)
		{
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] Wave Max NPCs reached (%d/%d). Pausing spawn."), TotalNPCsSpawnedInWave, MaxNPCsPerWave);
			}
			StopSpawning();
			return;
		}

		if (ActiveNPCs.Num() < MaxActiveNPCs)
		{
			if (APawn* SpawnedNPC = SpawnNPC())
			{
				TotalNPCsSpawnedInWave++;
			}
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
		FString Msg = TEXT("[GSNPCManager] SpawnNPC FAILED: NPCPawnClass is NULL! Assign your NPC Blueprint in Details panel.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
		return nullptr;
	}

	if (SpawnPoints.Num() == 0)
	{
		FString Msg = TEXT("[GSNPCManager] SpawnNPC FAILED: SpawnPoints array is empty! Place target points in map and assign them in Details panel.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
		return nullptr;
	}

	int32 FreeQueueIdx = INDEX_NONE;
	if (QueueSpots.Num() > 0)
	{
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

		if (AGSPawn* GSPawn = Cast<AGSPawn>(NewNPC))
		{
			if (NPCCharacterDataAssets.Num() > 0)
			{
				int32 DataIdx = FMath::RandRange(0, NPCCharacterDataAssets.Num() - 1);
				if (UGSCharacterDataAsset* ChosenData = NPCCharacterDataAssets[DataIdx])
				{
					GSPawn->ApplyCharacterDataAsset(ChosenData);
				}
			}
		}

		if (FreeQueueIdx != INDEX_NONE && QueueOccupants.IsValidIndex(FreeQueueIdx))
		{
			QueueOccupants[FreeQueueIdx] = NewNPC;
		}

		if (UGSNPCComponent* NPCComp = NewNPC->FindComponentByClass<UGSNPCComponent>())
		{
			NPCComp->SetNPCState(ENPCState::Entering);
			if (FreeQueueIdx != INDEX_NONE && QueueSpots.IsValidIndex(FreeQueueIdx))
			{
				NPCComp->SetAssignedTargetSpot(QueueSpots[FreeQueueIdx]);
			}
		}
		else if (UGSNPCComponentAlphaTest* AlphaComp = NewNPC->FindComponentByClass<UGSNPCComponentAlphaTest>())
		{
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("[GSNPCManager] Spawned NPC %s with GSNPCComponentAlphaTest"), *NewNPC->GetName());
			}
		}
		else if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] SpawnNPC WARNING: Spawned Pawn %s does not have a GSNPCComponent or GSNPCComponentAlphaTest!"), *NewNPC->GetName());
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
		AActor* TargetSpot = TableSpots.IsValidIndex(FreeTableIdx) ? TableSpots[FreeTableIdx].Get() : nullptr;
		NPCComp->SetAssignedTargetSpot(TargetSpot);
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
