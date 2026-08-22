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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TArray<TObjectPtr<AActor>> SpawnPoints;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TArray<TObjectPtr<AActor>> QueueSpots;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TArray<TObjectPtr<AActor>> TableSpots;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TObjectPtr<AActor> ExitSpot;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TSubclassOf<APawn> NPCPawnClass;

	/** Optional pool of character data assets (containing stats, abilities, and skins) assigned randomly to spawned NPCs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Data")
	TArray<TObjectPtr<class UGSCharacterDataAsset>> NPCCharacterDataAssets;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	float SpawnInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	int32 MaxActiveNPCs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Wave")
	int32 MaxNPCsPerWave;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Spawner|Wave")
	int32 TotalNPCsSpawnedInWave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Wave")
	bool bAutoListenToGamePhase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Debug")
	bool bShowDebugLogs;

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
	bool AssignTableToNPC(APawn* NPC);


	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void SendNPCToExit(APawn* NPC);


	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void ShiftQueue();


	UFUNCTION(BlueprintPure, Category = "NPC Spawner")
	const TArray<APawn*>& GetActiveNPCs() const { return ActiveNPCs; }

private:

	UPROPERTY()
	TArray<TObjectPtr<APawn>> ActiveNPCs;


	UPROPERTY()
	TArray<TObjectPtr<APawn>> QueueOccupants;


	UPROPERTY()
	TArray<TObjectPtr<APawn>> TableOccupants;

	FTimerHandle SpawnTimerHandle;

	void HandlePeriodicSpawn();
};
