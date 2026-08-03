#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "Core/GSNPCManager.h"
#include "GSStateTreeTask_ControlNPCSpawner.generated.h"

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_ControlNPCSpawnerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "If true, starts NPC spawner; if false, stops spawner"))
	bool bStartSpawning = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "If true, resets TotalNPCsSpawnedInWave to 0"))
	bool bResetWaveProgress = true;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Control NPC Spawner", Category = "Game Cycle"))
struct PROJECTF_API FGSStateTreeTask_ControlNPCSpawner : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_ControlNPCSpawnerInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
