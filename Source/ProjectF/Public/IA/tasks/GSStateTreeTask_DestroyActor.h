#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GSStateTreeTask_DestroyActor.generated.h"

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_DestroyActorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Destroy Actor", Category = "NPC"))
struct PROJECTF_API FGSStateTreeTask_DestroyActor : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_DestroyActorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
