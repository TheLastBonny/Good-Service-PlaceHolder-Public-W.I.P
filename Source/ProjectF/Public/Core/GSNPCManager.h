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
	// Spawn points where NPCs appear in the world
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	// Ordered array of queue spots (Index 0 is the front of the queue / service desk)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TArray<TObjectPtr<AActor>> QueueSpots;

	// Dining tables where NPCs go to eat after receiving their food
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TArray<TObjectPtr<AActor>> TableSpots;

	// Spot where NPCs walk to despawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TObjectPtr<AActor> ExitSpot;

	// Pawn class to spawn for NPCs (should inherit from AGSPawn)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TSubclassOf<APawn> NPCPawnClass;

	// Seconds between NPC spawns
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	float SpawnInterval;

	// Maximum number of active NPCs allowed in the restaurant at once
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	int32 MaxActiveNPCs;

	// Starts spawning NPCs periodically
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void StartSpawning();

	// Stops spawning NPCs
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void StopSpawning();

	// Spawns a single NPC at a random spawn point and adds them to the queue line
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	APawn* SpawnNPC();

	// Assigns a table to an NPC, freeing their queue spot and shifting the queue forward
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	bool AssignTableToNPC(APawn* NPC);

	// Removes the NPC from queue/table slots and commands them to walk to the exit spot
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void SendNPCToExit(APawn* NPC);

	// Shifts the queue line forward if the front customer leaves or moves to a table
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void ShiftQueue();

	// Getters for spawner tracking
	UFUNCTION(BlueprintPure, Category = "NPC Spawner")
	const TArray<APawn*>& GetActiveNPCs() const { return ActiveNPCs; }

private:
	// Tracks all spawned NPCs currently in the game
	UPROPERTY()
	TArray<TObjectPtr<APawn>> ActiveNPCs;

	// Maps 1-to-1 with QueueSpots to track who is occupying which slot
	UPROPERTY()
	TArray<TObjectPtr<APawn>> QueueOccupants;

	// Maps 1-to-1 with TableSpots to track who is occupying which dining table
	UPROPERTY()
	TArray<TObjectPtr<APawn>> TableOccupants;

	FTimerHandle SpawnTimerHandle;

	void HandlePeriodicSpawn();
};
