#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSNPCManager.generated.h"

UCLASS()
class PROJECTF_API AGSNPCManager : public AActor
{
	GENERATED_BODY()

public:
	AGSNPCManager();

protected:
	virtual void BeginPlay() override;

public:

	// ==========================================
	// CONFIGURATION (ESSENTIAL FOR SPAWNER)
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TSubclassOf<APawn> NPCPawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner", meta = (ClampMin = "0.5"))
	float SpawnInterval;

	/** Number of NPCs to spawn per cycle (e.g. 2 NPCs every 20 seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner", meta = (ClampMin = "1"))
	int32 NPCsPerSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner", meta = (ClampMin = "1"))
	int32 MaxActiveNPCs;

	/** Optional pool of character data assets (containing stats, abilities, and skins) assigned randomly to spawned NPCs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Data")
	TArray<TObjectPtr<class UGSCharacterDataAsset>> NPCCharacterDataAssets;

	/** Total max NPCs to spawn in this wave. Set to 0 (or negative) for continuous infinite looping! */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Wave", meta = (ClampMin = "0"))
	int32 MaxNPCsPerWave;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Spawner|Wave")
	int32 TotalNPCsSpawnedInWave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Wave")
	bool bAutoListenToGamePhase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Debug")
	bool bShowDebugLogs;

	// ==========================================
	// LEGACY / OPTIONAL SPOTS (NOT REQUIRED FOR SMART OBJECTS)
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Legacy")
	TArray<TObjectPtr<AActor>> QueueSpots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Legacy")
	TArray<TObjectPtr<AActor>> TableSpots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Legacy")
	TObjectPtr<AActor> ExitSpot;

	// ==========================================
	// SPAWNER CONTROL FUNCTIONS
	// ==========================================
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner|Wave")
	void ResetWaveProgress();

	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void StopSpawning();

	UFUNCTION()
	void HandleGamePhaseTagChanged(FGameplayTag NewPhaseTag);

	UFUNCTION()
	void HandleGamePhaseChanged(EGSGamePhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	APawn* SpawnNPC();

	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void UnregisterNPC(APawn* NPC);

	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	bool AssignTableToNPC(APawn* NPC);

	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void SendNPCToExit(APawn* NPC);

	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void ShiftQueue();

	UFUNCTION(BlueprintPure, Category = "NPC Spawner")
	const TArray<APawn*>& GetActiveNPCs() const { return ActiveNPCs; }

private:
	UFUNCTION()
	void HandleNPCDestroyed(AActor* DestroyedActor);

	void CleanStaleActiveNPCs();

	UPROPERTY()
	TArray<TObjectPtr<APawn>> ActiveNPCs;

	UPROPERTY()
	TArray<TObjectPtr<APawn>> QueueOccupants;

	UPROPERTY()
	TArray<TObjectPtr<APawn>> TableOccupants;

	FTimerHandle SpawnTimerHandle;

	void HandlePeriodicSpawn();
};
