
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "GSStationDataAsset.generated.h"

class UGameplayEffect;
class UAttributeSet;
class AGSUtilityStation;

/** Conditional effect entry: applied to an item when it reaches a specific state tag. */
USTRUCT(BlueprintType)
struct FGSConditionalEffectEntry
{
	GENERATED_BODY()

	/** State tag that triggers effect application. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	FGameplayTag StateTag;

	/** Gameplay effects to apply when StateTag becomes active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;
};

/** Spawner configuration for a specific station socket. */
USTRUCT(BlueprintType)
struct FGSSocketSpawnerConfig
{
	GENERATED_BODY()

	/** Socket where the item will be spawned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	FName TargetSocket = NAME_None;

	/** Actor / item class to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<AActor> ItemClass = nullptr;

	/** Interval in seconds to respawn if socket is free. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0.1"))
	float SpawnInterval = 5.0f;

	/** Initial delay before first spawn attempt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0.0"))
	float InitialDelay = 0.5f;

	/** If true, spawns immediately on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bSpawnImmediatelyOnStart = true;
};

/** Base class for configurable station modules. */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTF_API UGSStationModule : public UObject
{
	GENERATED_BODY()

public:
	virtual void InitializeModule(AGSUtilityStation* Station) {}
	virtual void ShutdownModule(AGSUtilityStation* Station) {}
	virtual void OnItemAdded(AGSUtilityStation* Station, AActor* Item) {}
	virtual void OnItemRemoved(AGSUtilityStation* Station, AActor* Item) {}
};

/** Spawner module: periodically generates items in designated sockets when vacant. */
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Spawner Module"))
class PROJECTF_API UGSStationModule_Spawner : public UGSStationModule
{
	GENERATED_BODY()

public:
	UGSStationModule_Spawner();

	/** If true, discovers all sockets and keeps them stocked with AutoFillItemClass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutoFill Spawner")
	bool bAutoFillAllStationSockets = false;

	/** Item class to automatically spawn in all sockets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutoFill Spawner", meta = (EditCondition = "bAutoFillAllStationSockets"))
	TSubclassOf<AActor> AutoFillItemClass = nullptr;

	/** Interval in seconds to replenish vacant sockets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutoFill Spawner", meta = (EditCondition = "bAutoFillAllStationSockets", ClampMin = "0.1"))
	float AutoFillSpawnInterval = 5.0f;

	/** Initial delay before first spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutoFill Spawner", meta = (EditCondition = "bAutoFillAllStationSockets", ClampMin = "0.0"))
	float AutoFillInitialDelay = 0.5f;

	/** If true, immediately fills vacant sockets on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutoFill Spawner", meta = (EditCondition = "bAutoFillAllStationSockets"))
	bool bAutoFillSpawnImmediatelyOnStart = true;

	/** If true, picks vacant sockets in random order when restocking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutoFill Spawner", meta = (EditCondition = "bAutoFillAllStationSockets"))
	bool bRandomizeSpawnOrder = true;

	/** Manual socket configurations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Manual Sockets Spawner")
	TArray<FGSSocketSpawnerConfig> SpawnerConfigs;

	virtual void InitializeModule(AGSUtilityStation* Station) override;
	virtual void ShutdownModule(AGSUtilityStation* Station) override;
	virtual void OnItemRemoved(AGSUtilityStation* Station, AActor* Item) override;

	void TrySpawnForConfig(TWeakObjectPtr<AGSUtilityStation> WeakStation, int32 ConfigIndex);
	void TryAutoFillSpawn(TWeakObjectPtr<AGSUtilityStation> WeakStation);

private:
	TMap<int32, FTimerHandle> ActiveTimerHandles;
	FTimerHandle AutoFillTimerHandle;
};

USTRUCT(BlueprintType)
struct FGSStationDetails
{
	GENERATED_BODY()

	/** Effects applied to items immediately upon entering the station. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	/** Conditional effects applied when item achieves specific state tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<FGSConditionalEffectEntry> ConditionalEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	/** If true, placed items are hidden visually. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Station Configuration")
	bool bHidePlacedItems = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
	FVector HitBoxSize = FVector(100.0f, 100.0f, 100.0f);
};

UCLASS(BlueprintType)
class PROJECTF_API UGSStationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSStationDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Station Configuration")
	FGSStationDetails StationDetails;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Modules")
	TArray<TObjectPtr<UGSStationModule>> Modules;
};
