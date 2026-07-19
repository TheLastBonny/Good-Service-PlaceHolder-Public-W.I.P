#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GSStateTreeTask_ChooseRandomOrder.generated.h"

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_ChooseRandomOrderInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;
};


USTRUCT(BlueprintType, meta = (DisplayName = "GS Choose Random Order"))
struct PROJECTF_API FGSStateTreeTask_ChooseRandomOrder : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_ChooseRandomOrderInstanceData;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
