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

	NPCPawnClass = nullptr;
	SpawnInterval = 6.0f;
	NPCsPerSpawn = 1;
	MaxActiveNPCs = 4;
	MaxNPCsPerWave = 0; // 0 = Continuous infinite loop
	TotalNPCsSpawnedInWave = 0;
	bAutoListenToGamePhase = true;
	ExitSpot = nullptr;
	bShowDebugLogs = false;
}

void AGSNPCManager::BeginPlay()
{
	Super::BeginPlay();

	if (QueueSpots.Num() > 0)
	{
		QueueOccupants.Init(nullptr, QueueSpots.Num());
	}
	if (TableSpots.Num() > 0)
	{
		TableOccupants.Init(nullptr, TableSpots.Num());
	}

	if (UWorld* World = GetWorld())
	{
		if (AGSGameState* GSGameState = Cast<AGSGameState>(World->GetGameState()))
		{
			GSGameState->RegisterNPCManager(this);
			if (bAutoListenToGamePhase)
			{
				GSGameState->OnPrimaryPhaseTagChanged.AddDynamic(this, &AGSNPCManager::HandleGamePhaseTagChanged);
				GSGameState->OnGamePhaseChanged.AddDynamic(this, &AGSNPCManager::HandleGamePhaseChanged);

				// If the game started already in RoundInProgress, kick off spawning immediately
				if (GSGameState->GetCurrentPhase() == EGSGamePhase::RoundInProgress ||
					GSGameState->GetPrimaryPhaseTag() == GSGameplayTags::GamePhase_Core_RoundInProgress)
				{
					if (bShowDebugLogs)
					{
						UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] BeginPlay: GameState already in RoundInProgress -> Starting Spawning immediately."));
					}
					StartSpawning();
				}
			}
		}
	}

	if (bShowDebugLogs)
	{
		FString SetupMsg = FString::Printf(TEXT("[GSNPCManager] Ready! Class: %s | Spawns: %d | Batch: %d every %.1fs | MaxActive: %d | MaxWave: %d (0=Inf) | AutoPhase: %s"),
			NPCPawnClass ? *NPCPawnClass->GetName() : TEXT("NONE!"),
			SpawnPoints.Num(),
			NPCsPerSpawn,
			SpawnInterval,
			MaxActiveNPCs,
			MaxNPCsPerWave,
			bAutoListenToGamePhase ? TEXT("TRUE") : TEXT("FALSE"));

		UE_LOG(LogTemp, Log, TEXT("%s"), *SetupMsg);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, SetupMsg);

			if (!NPCPawnClass)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("[GSNPCManager] ⚠️ WARNING: NPCPawnClass is NOT assigned in Details panel!"));
			}
			if (SpawnPoints.Num() == 0)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("[GSNPCManager] ⚠️ WARNING: SpawnPoints array is EMPTY in Details panel!"));
			}
		}
	}
}

void AGSNPCManager::ResetWaveProgress()
{
	TotalNPCsSpawnedInWave = 0;
	if (bShowDebugLogs)
	{
		FString Msg = TEXT("[GSNPCManager] Wave progress reset to 0");
		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan, Msg);
	}
}

void AGSNPCManager::HandleGamePhaseTagChanged(FGameplayTag NewPhaseTag)
{
	if (!HasAuthority()) return;

	if (NewPhaseTag == GSGameplayTags::GamePhase_Core_RoundInProgress)
	{
		if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] PhaseTag -> %s: Starting Spawner!"), *NewPhaseTag.ToString());
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Msg);
		}
		ResetWaveProgress();
		StartSpawning();
	}
	else if (NewPhaseTag == GSGameplayTags::GamePhase_Core_WaitingToStart || NewPhaseTag == GSGameplayTags::GamePhase_Core_RoundOver)
	{
		if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] PhaseTag -> %s: Stopping Spawner."), *NewPhaseTag.ToString());
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, Msg);
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
			FString Msg = TEXT("[GSNPCManager] PhaseEnum -> RoundInProgress: Starting Spawner!");
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Msg);
		}
		ResetWaveProgress();
		StartSpawning();
	}
	else if (NewPhase == EGSGamePhase::WaitingToStart || NewPhase == EGSGamePhase::RoundOver)
	{
		if (bShowDebugLogs)
		{
			FString Msg = TEXT("[GSNPCManager] PhaseEnum -> WaitingToStart/RoundOver: Stopping Spawner.");
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, Msg);
		}
		StopSpawning();
	}
}

void AGSNPCManager::StartSpawning()
{
	if (HasAuthority() && NPCPawnClass)
	{
		// Clean any stale references before starting
		CleanStaleActiveNPCs();

		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AGSNPCManager::HandlePeriodicSpawn, SpawnInterval, true);
		if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] ▶️ Spawner STARTED (Interval: %.1fs | MaxActive: %d | WaveCap: %d)"),
				SpawnInterval, MaxActiveNPCs, MaxNPCsPerWave);
			UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, Msg);
		}
	}
	else if (bShowDebugLogs)
	{
		FString Msg = FString::Printf(TEXT("[GSNPCManager] ❌ StartSpawning FAILED: HasAuthority=%d, NPCPawnClass=%s"),
			HasAuthority(), NPCPawnClass ? *NPCPawnClass->GetName() : TEXT("NULL"));
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Red, Msg);
	}
}

void AGSNPCManager::StopSpawning()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		if (bShowDebugLogs)
		{
			FString Msg = TEXT("[GSNPCManager] ⏸️ Spawner STOPPED (Timer Cleared)");
			UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, Msg);
		}
	}
}

void AGSNPCManager::CleanStaleActiveNPCs()
{
	int32 RemovedCount = ActiveNPCs.RemoveAll([](const TObjectPtr<APawn>& PawnPtr)
	{
		return !IsValid(PawnPtr) || PawnPtr->IsActorBeingDestroyed();
	});

	if (RemovedCount > 0 && bShowDebugLogs)
	{
		FString Msg = FString::Printf(TEXT("[GSNPCManager] Cleaned %d dead/invalid NPC pointers. Active count now: %d"),
			RemovedCount, ActiveNPCs.Num());
		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Magenta, Msg);
	}
}

void AGSNPCManager::HandlePeriodicSpawn()
{
	if (!HasAuthority()) return;

	CleanStaleActiveNPCs();

	// Check optional wave limit (0 or negative means infinite loop)
	if (MaxNPCsPerWave > 0 && TotalNPCsSpawnedInWave >= MaxNPCsPerWave)
	{
		if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] 🛑 Wave cap reached (%d/%d NPCs). Pausing spawn."), TotalNPCsSpawnedInWave, MaxNPCsPerWave);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Orange, Msg);
		}
		StopSpawning();
		return;
	}

	int32 BatchSize = FMath::Max(1, NPCsPerSpawn);
	int32 SpawnedThisCycle = 0;

	for (int32 i = 0; i < BatchSize; ++i)
	{
		if (MaxNPCsPerWave > 0 && TotalNPCsSpawnedInWave >= MaxNPCsPerWave)
		{
			break;
		}

		if (ActiveNPCs.Num() < MaxActiveNPCs)
		{
			if (APawn* SpawnedNPC = SpawnNPC())
			{
				TotalNPCsSpawnedInWave++;
				SpawnedThisCycle++;
			}
			else
			{
				break;
			}
		}
		else
		{
			if (bShowDebugLogs && SpawnedThisCycle == 0)
			{
				FString Msg = FString::Printf(TEXT("[GSNPCManager] ⏳ Store at FULL capacity: %d/%d active NPCs (waiting for departure)"), ActiveNPCs.Num(), MaxActiveNPCs);
				UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Msg);
			}
			break;
		}
	}
}

APawn* AGSNPCManager::SpawnNPC()
{
	if (!HasAuthority())
	{
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GSNPCManager] SpawnNPC FAILED: No Authority"));
		}
		return nullptr;
	}

	if (!NPCPawnClass)
	{
		FString Msg = TEXT("[GSNPCManager] ❌ SpawnNPC FAILED: NPCPawnClass is NULL! Select your NPC Blueprint in Details.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
		return nullptr;
	}

	if (SpawnPoints.Num() == 0)
	{
		FString Msg = TEXT("[GSNPCManager] ❌ SpawnNPC FAILED: SpawnPoints array is empty! Assign TargetPoints in Details.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
		return nullptr;
	}

	// Legacy queue spot check (only if QueueSpots is actually used)
	int32 FreeQueueIdx = INDEX_NONE;
	if (QueueSpots.Num() > 0)
	{
		if (QueueOccupants.Num() != QueueSpots.Num())
		{
			QueueOccupants.Init(nullptr, QueueSpots.Num());
		}

		for (int32 i = 0; i < QueueOccupants.Num(); ++i)
		{
			if (QueueOccupants[i] == nullptr || !IsValid(QueueOccupants[i]))
			{
				FreeQueueIdx = i;
				break;
			}
		}
	}

	// Select a random spawn point
	TArray<AActor*> ValidSpawnPoints;
	for (AActor* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			ValidSpawnPoints.Add(Point);
		}
	}

	if (ValidSpawnPoints.Num() == 0)
	{
		if (bShowDebugLogs)
		{
			FString Msg = TEXT("[GSNPCManager] ❌ SpawnNPC FAILED: All assigned SpawnPoints are NULL/Invalid!");
			UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
		}
		return nullptr;
	}

	int32 RandomSpawnIdx = FMath::RandRange(0, ValidSpawnPoints.Num() - 1);
	AActor* SpawnPoint = ValidSpawnPoints[RandomSpawnIdx];

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector SpawnLoc = SpawnPoint->GetActorLocation();
	FRotator SpawnRot = SpawnPoint->GetActorRotation();

	if (APawn* NewNPC = GetWorld()->SpawnActor<APawn>(NPCPawnClass, SpawnLoc, SpawnRot, SpawnParams))
	{
		ActiveNPCs.Add(NewNPC);

		// Auto-cleanup when destroyed in the world
		NewNPC->OnDestroyed.AddDynamic(this, &AGSNPCManager::HandleNPCDestroyed);

		// Apply Character Data Asset if available
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

		// Legacy queue spot assignment if available
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

		if (bShowDebugLogs)
		{
			FString Msg = FString::Printf(TEXT("[GSNPCManager] 🟢 SPAWNED %s at '%s' (Active: %d/%d | Wave: %d)"),
				*NewNPC->GetName(),
				*SpawnPoint->GetName(),
				ActiveNPCs.Num(),
				MaxActiveNPCs,
				TotalNPCsSpawnedInWave + 1);
			UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, Msg);
		}

		return NewNPC;
	}

	if (bShowDebugLogs)
	{
		FString Msg = TEXT("[GSNPCManager] ❌ SpawnNPC FAILED: SpawnActor returned NULL (Check collision or Pawn class)");
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, Msg);
	}

	return nullptr;
}

void AGSNPCManager::HandleNPCDestroyed(AActor* DestroyedActor)
{
	if (APawn* DestroyedPawn = Cast<APawn>(DestroyedActor))
	{
		UnregisterNPC(DestroyedPawn);
	}
}

void AGSNPCManager::UnregisterNPC(APawn* NPC)
{
	if (!NPC) return;

	ActiveNPCs.Remove(NPC);

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

	if (bShowDebugLogs)
	{
		FString Msg = FString::Printf(TEXT("[GSNPCManager] 🗑️ Unregistered %s (Remaining Active: %d/%d)"),
			*NPC->GetName(), ActiveNPCs.Num(), MaxActiveNPCs);
		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.5f, FColor::Magenta, Msg);
	}
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

	// Immediately unregister to free up capacity for the next customer
	UnregisterNPC(NPC);

	if (bShowDebugLogs)
	{
		FString Msg = FString::Printf(TEXT("[GSNPCManager] 🚪 NPC %s sent to exit -> Slot freed! Active: %d/%d"),
			*NPC->GetName(), ActiveNPCs.Num(), MaxActiveNPCs);
		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan, Msg);
	}

	if (UGSNPCComponent* NPCComp = Cast<UGSNPCComponent>(NPC->GetComponentByClass(UGSNPCComponent::StaticClass())))
	{
		NPCComp->SetNPCState(ENPCState::Leaving);
		if (ExitSpot)
		{
			NPCComp->SetAssignedTargetSpot(ExitSpot);
		}
	}
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
		if (NPC != nullptr && IsValid(NPC))
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
