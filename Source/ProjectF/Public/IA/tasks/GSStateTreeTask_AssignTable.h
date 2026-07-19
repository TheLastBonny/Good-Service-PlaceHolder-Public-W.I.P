#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GSStateTreeTask_AssignTable.generated.h"

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_AssignTableInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	bool bSuccess = false;
};


USTRUCT(BlueprintType, meta = (DisplayName = "GS Assign Table"))
struct PROJECTF_API FGSStateTreeTask_AssignTable : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_AssignTableInstanceData;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
