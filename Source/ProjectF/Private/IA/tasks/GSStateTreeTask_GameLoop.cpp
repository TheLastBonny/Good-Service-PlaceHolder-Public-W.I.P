#include "IA/tasks/GSStateTreeTask_GameLoop.h"
#include "StateTreeExecutionContext.h"
#include "Core/GSGameState.h"
#include "Engine/World.h"

// Helper function to resolve GameState context
static AGSGameState* ResolveGameState(AGSGameState* ContextGameState, FStateTreeExecutionContext& Context)
{
	if (ContextGameState)
	{
		return ContextGameState;
	}
	if (UWorld* World = Context.GetWorld())
	{
		return Cast<AGSGameState>(World->GetGameState());
	}
	return nullptr;
}

// ==========================================
// TASK 1: Set Game Phase
// ==========================================

EStateTreeRunStatus FGSStateTreeTask_SetGamePhase::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		if (InstanceData.SetRemainingTime > 0.0f)
		{
			GameState->SetRemainingTime(InstanceData.SetRemainingTime);
		}

		if (InstanceData.PhaseTag.IsValid())
		{
			GameState->SetPrimaryPhaseTag(InstanceData.PhaseTag);
		}

		GameState->SetGamePhase(InstanceData.TargetPhase);
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}


// ==========================================
// TASK 2: Add Game Phase Tag
// ==========================================

EStateTreeRunStatus FGSStateTreeTask_AddGamePhase::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		if (InstanceData.PhaseTag.IsValid())
		{
			GameState->AddPhaseTag(InstanceData.PhaseTag);
		}
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}


// ==========================================
// TASK 3: Remove Game Phase Tag
// ==========================================

EStateTreeRunStatus FGSStateTreeTask_RemoveGamePhase::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		if (InstanceData.PhaseTag.IsValid())
		{
			GameState->RemovePhaseTag(InstanceData.PhaseTag);
		}
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}


// ==========================================
// TASK 4: Set Timer
// ==========================================

EStateTreeRunStatus FGSStateTreeTask_SetTimer::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		GameState->SetRemainingTime(InstanceData.Duration);

		if (InstanceData.bStartTimerImmediately)
		{
			GameState->StartRoundTimer();
		}

		if (InstanceData.bAutoCompleteOnZero && GameState->GetRemainingTime() <= 0.0f)
		{
			return EStateTreeRunStatus::Succeeded;
		}

		return EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FGSStateTreeTask_SetTimer::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		if (InstanceData.bAutoCompleteOnZero && GameState->GetRemainingTime() <= 0.0f)
		{
			return EStateTreeRunStatus::Succeeded;
		}
		return EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}

void FGSStateTreeTask_SetTimer::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}


// ==========================================
// TASK 5: Start Timer
// ==========================================

EStateTreeRunStatus FGSStateTreeTask_StartTimer::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		if (InstanceData.OptionalSetTime > 0.0f)
		{
			GameState->SetRemainingTime(InstanceData.OptionalSetTime);
		}

		GameState->StartRoundTimer();
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}


// ==========================================
// TASK 6: Stop Timer
// ==========================================

EStateTreeRunStatus FGSStateTreeTask_StopTimer::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		GameState->StopRoundTimer();
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}


// ==========================================
// TASK 7: Add Timer
// ==========================================

EStateTreeRunStatus FGSStateTreeTask_AddTimer::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		GameState->AddRemainingTime(InstanceData.AdditionalTime);
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}


// ==========================================
// LEGACY BACKWARD COMPATIBILITY
// ==========================================

EStateTreeRunStatus FGSStateTreeTask_ControlRoundTimer::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority())
	{
		if (InstanceData.OptionalSetTime > 0.0f)
		{
			GameState->SetRemainingTime(InstanceData.OptionalSetTime);
		}

		GameState->StartRoundTimer();
		return EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}

void FGSStateTreeTask_ControlRoundTimer::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGSGameState* GameState = ResolveGameState(InstanceData.GameState, Context);

	if (GameState && GameState->HasAuthority() && InstanceData.bStopTimerOnExit)
	{
		GameState->StopRoundTimer();
	}
}

