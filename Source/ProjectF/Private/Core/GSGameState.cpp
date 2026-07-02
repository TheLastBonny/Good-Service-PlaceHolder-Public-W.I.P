#include "Core/GSGameState.h"
#include "AbilitySystemComponent.h"
#include "Attributes/GSMoneyAttributeSet.h"
#include "Core/GSNPCManager.h"
#include "Net/UnrealNetwork.h"

AGSGameState::AGSGameState()
{
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	MoneyAttributeSet = CreateDefaultSubobject<UGSMoneyAttributeSet>(TEXT("MoneyAttributeSet"));

	CurrentPhase = EGSGamePhase::WaitingToStart;
	RemainingTime = 0.0f;
	NPCManager = nullptr;
}

void AGSGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AGSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGSGameState, CurrentPhase);
	DOREPLIFETIME(AGSGameState, RemainingTime);
}

UAbilitySystemComponent* AGSGameState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

int32 AGSGameState::GetMoney() const
{
	if (MoneyAttributeSet)
	{
		return FMath::RoundToInt(MoneyAttributeSet->GetMoney());
	}
	return 0;
}

float AGSGameState::GetMoneyMultiplier() const
{
	if (MoneyAttributeSet)
	{
		return MoneyAttributeSet->GetMoneyMultiplier();
	}
	return 1.0f;
}

void AGSGameState::AddMoneyDirectly(float BaseAmount)
{
	if (HasAuthority() && MoneyAttributeSet && BaseAmount > 0.0f)
	{
		float CurrentMoney = MoneyAttributeSet->GetMoney();
		float Multiplier = MoneyAttributeSet->GetMoneyMultiplier();
		float FinalAdd = BaseAmount * Multiplier;
		
		AbilitySystemComponent->SetNumericAttributeBase(UGSMoneyAttributeSet::GetMoneyAttribute(), CurrentMoney + FinalAdd);
		
		// Broadcast changes locally on the server (clients will receive it via OnRep_Money)
		OnMoneyChanged.Broadcast(FMath::RoundToInt(MoneyAttributeSet->GetMoney()));
	}
}

void AGSGameState::SetGamePhase(EGSGamePhase NewPhase)
{
	if (HasAuthority() && CurrentPhase != NewPhase)
	{
		EGSGamePhase OldPhase = CurrentPhase;
		CurrentPhase = NewPhase;
		
		// Trigger local event on server
		OnRep_CurrentPhase(OldPhase);

		// Handle phase logic
		if (CurrentPhase == EGSGamePhase::RoundInProgress)
		{
			StartRoundTimer();
			if (NPCManager)
			{
				NPCManager->StartSpawning();
			}
		}
		else if (CurrentPhase == EGSGamePhase::RoundOver)
		{
			StopRoundTimer();
			if (NPCManager)
			{
				NPCManager->StopSpawning();
			}
		}
		else if (CurrentPhase == EGSGamePhase::WaitingToStart)
		{
			StopRoundTimer();
			if (NPCManager)
			{
				NPCManager->StopSpawning();
			}
		}
	}
}

void AGSGameState::SetRemainingTime(float NewTime)
{
	if (HasAuthority())
	{
		RemainingTime = FMath::Max(NewTime, 0.0f);
		OnRemainingTimeChanged.Broadcast(RemainingTime);
	}
}

void AGSGameState::StartRoundTimer()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(RoundTimerHandle, this, &AGSGameState::DecrementRoundTime, 1.0f, true);
	}
}

void AGSGameState::StopRoundTimer()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(RoundTimerHandle);
	}
}

void AGSGameState::DecrementRoundTime()
{
	if (HasAuthority())
	{
		if (RemainingTime > 0.0f)
		{
			SetRemainingTime(RemainingTime - 1.0f);
		}
		
		if (RemainingTime <= 0.0f)
		{
			StopRoundTimer();
			SetGamePhase(EGSGamePhase::RoundOver);
		}
	}
}

void AGSGameState::RegisterNPCManager(AGSNPCManager* Manager)
{
	NPCManager = Manager;
	
	// If the round is already in progress, start spawning immediately
	if (HasAuthority() && NPCManager && CurrentPhase == EGSGamePhase::RoundInProgress)
	{
		NPCManager->StartSpawning();
	}
}

void AGSGameState::OnRep_CurrentPhase(EGSGamePhase OldPhase)
{
	OnGamePhaseChanged.Broadcast(CurrentPhase);
}

void AGSGameState::OnRep_RemainingTime(float OldTime)
{
	OnRemainingTimeChanged.Broadcast(RemainingTime);
}
