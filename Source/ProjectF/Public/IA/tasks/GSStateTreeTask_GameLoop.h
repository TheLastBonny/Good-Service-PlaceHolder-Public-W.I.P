#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameState.h"
#include "GSStateTreeTask_GameLoop.generated.h"

// ==========================================
// TASK 1: Set Game Phase (Primary Tag / Enum)
// ==========================================

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_SetGamePhaseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AGSGameState> GameState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Legacy enum phase to set."))
	EGSGamePhase TargetPhase = EGSGamePhase::RoundInProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Primary GameplayTag phase to set (e.g. GamePhase.Core.RoundInProgress)."))
	FGameplayTag PhaseTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "If greater than 0, sets the round remaining time when entering this phase."))
	float SetRemainingTime = 0.0f;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Set Game Phase", Category = "Game Loop"))
struct PROJECTF_API FGSStateTreeTask_SetGamePhase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_SetGamePhaseInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};


// ==========================================
// TASK 2: Add Game Phase (Secondary Tag)
// ==========================================

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_AddGamePhaseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AGSGameState> GameState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Secondary GameplayTag phase or event to activate."))
	FGameplayTag PhaseTag;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Add Game Phase Tag", Category = "Game Loop"))
struct PROJECTF_API FGSStateTreeTask_AddGamePhase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_AddGamePhaseInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};


// ==========================================
// TASK 3: Remove Game Phase (Secondary Tag)
// ==========================================

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_RemoveGamePhaseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AGSGameState> GameState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Secondary GameplayTag phase or event to deactivate."))
	FGameplayTag PhaseTag;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Remove Game Phase Tag", Category = "Game Loop"))
struct PROJECTF_API FGSStateTreeTask_RemoveGamePhase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_RemoveGamePhaseInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};


// ==========================================
// TASK 4: Set Timer (Runs Countdown & Completes on Zero)
// ==========================================

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_SetTimerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AGSGameState> GameState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Duration in seconds for this timer."))
	float Duration = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "If true, starts the countdown automatically on enter state."))
	bool bStartTimerImmediately = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "If true, returns Succeeded when remaining time reaches 0."))
	bool bAutoCompleteOnZero = true;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Set Timer", Category = "Game Loop"))
struct PROJECTF_API FGSStateTreeTask_SetTimer : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_SetTimerInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};


// ==========================================
// TASK 5: Start Timer
// ==========================================

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_StartTimerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AGSGameState> GameState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "If > 0, sets remaining time before starting."))
	float OptionalSetTime = 0.0f;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Start Timer", Category = "Game Loop"))
struct PROJECTF_API FGSStateTreeTask_StartTimer : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_StartTimerInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};


// ==========================================
// TASK 6: Stop Timer
// ==========================================

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_StopTimerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AGSGameState> GameState = nullptr;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Stop Timer", Category = "Game Loop"))
struct PROJECTF_API FGSStateTreeTask_StopTimer : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_StopTimerInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};


// ==========================================
// TASK 7: Add Timer (Add Extra Seconds)
// ==========================================

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_AddTimerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AGSGameState> GameState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter", meta = (ToolTip = "Seconds to add to current remaining time (e.g. +10s bonus time)."))
	float AdditionalTime = 10.0f;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Add Timer", Category = "Game Loop"))
struct PROJECTF_API FGSStateTreeTask_AddTimer : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_AddTimerInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};


// ==========================================
// LEGACY BACKWARD COMPATIBILITY
// ==========================================

USTRUCT(BlueprintType)
struct PROJECTF_API FGSStateTreeTask_ControlRoundTimerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AGSGameState> GameState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter")
	float OptionalSetTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter")
	bool bStopTimerOnExit = true;
};

USTRUCT(BlueprintType, meta = (DisplayName = "GS Control Round Timer (Legacy)", Category = "Game Loop"))
struct PROJECTF_API FGSStateTreeTask_ControlRoundTimer : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGSStateTreeTask_ControlRoundTimerInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

