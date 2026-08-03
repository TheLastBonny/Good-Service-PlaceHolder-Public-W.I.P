#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GSStateTreeTask_ReleaseSmartObject.generated.h"

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_ReleaseSmartObjectInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Optional specific spot actor to release. If null, clears the spot assigned in GSNPCComponent."))
	TObjectPtr<AActor> TargetSpot = nullptr;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Release Smart Object", Category = "NPC"))
struct PROJECTF_API FGSStateTreeTask_ReleaseSmartObject : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_ReleaseSmartObjectInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
