#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GSStateTreeTask_SendToExit.generated.h"

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_SendToExitInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	TObjectPtr<AActor> ExitSpot = nullptr;
};


USTRUCT(BlueprintType, meta = (DisplayName = "GS Send To Exit", Category = "NPC"))
struct PROJECTF_API FGSStateTreeTask_SendToExit : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_SendToExitInstanceData;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
