#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "SmartObjectRuntime.h"
#include "GSStateTreeTask_ClaimSmartObject.generated.h"

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_ClaimSmartObjectInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Gameplay Tag required on the Smart Object (e.g. SmartObject.Seat.Dining)."))
	FGameplayTag SmartObjectTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Search radius in units around the actor to find Smart Objects."))
	float SearchRadius = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "If true, automatically unclaims/releases the Smart Object when exiting this StateTree state."))
	bool bReleaseOnExit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	TObjectPtr<AActor> AssignedTargetSpot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	FVector TargetLocation = FVector::ZeroVector;

	FSmartObjectClaimHandle ClaimHandle;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Claim Smart Object", Category = "NPC"))
struct PROJECTF_API FGSStateTreeTask_ClaimSmartObject : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_ClaimSmartObjectInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
