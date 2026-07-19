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


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	float SpawnInterval;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	int32 MaxActiveNPCs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Debug")
	bool bShowDebugLogs;


	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void StartSpawning();


	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void StopSpawning();


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
