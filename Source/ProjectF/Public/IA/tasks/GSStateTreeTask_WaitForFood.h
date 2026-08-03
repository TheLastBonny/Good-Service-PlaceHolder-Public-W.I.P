#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GSStateTreeTask_WaitForFood.generated.h"

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_WaitForFoodInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Wait For Food", Category = "NPC"))
struct PROJECTF_API FGSStateTreeTask_WaitForFood : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_WaitForFoodInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
