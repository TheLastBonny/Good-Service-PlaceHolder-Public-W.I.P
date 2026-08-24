#include "Core/GSGameState.h"
#include "AbilitySystemComponent.h"
#include "Attributes/GSMoneyAttributeSet.h"
#include "Core/GSNPCManager.h"
#include "Components/StateTreeComponent.h"
#include "Net/UnrealNetwork.h"

AGSGameState::AGSGameState()
{
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	MoneyAttributeSet = CreateDefaultSubobject<UGSMoneyAttributeSet>(TEXT("MoneyAttributeSet"));
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));

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
	DOREPLIFETIME(AGSGameState, PrimaryPhaseTag);
	DOREPLIFETIME(AGSGameState, ActivePhaseTags);
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
	if (HasAuthority() && MoneyAttributeSet && BaseAmount != 0.0f)
	{
		float CurrentMoney = MoneyAttributeSet->GetMoney();
		float FinalAdd = BaseAmount;
		if (BaseAmount > 0.0f)
		{
			float Multiplier = MoneyAttributeSet->GetMoneyMultiplier();
			FinalAdd = BaseAmount * Multiplier;
		}
		
		float NewMoney = FMath::Max(CurrentMoney + FinalAdd, 0.0f);
		AbilitySystemComponent->SetNumericAttributeBase(UGSMoneyAttributeSet::GetMoneyAttribute(), NewMoney);
		

		OnMoneyChanged.Broadcast(FMath::RoundToInt(NewMoney));
	}
}

void AGSGameState::SetGamePhase(EGSGamePhase NewPhase)
{
	if (HasAuthority() && CurrentPhase != NewPhase)
	{
		EGSGamePhase OldPhase = CurrentPhase;
		CurrentPhase = NewPhase;
		

		OnRep_CurrentPhase(OldPhase);


		if (CurrentPhase == EGSGamePhase::RoundInProgress)
		{
			if (NPCManager)
			{
				NPCManager->StartSpawning();
			}
		}
		else
		{
			if (NPCManager)
			{
				NPCManager->StopSpawning();
			}
		}
	}
}

void AGSGameState::SetPrimaryPhaseTag(FGameplayTag NewPhaseTag)
{
	if (HasAuthority() && PrimaryPhaseTag != NewPhaseTag)
	{
		FGameplayTag OldTag = PrimaryPhaseTag;
		PrimaryPhaseTag = NewPhaseTag;
		OnRep_PrimaryPhaseTag(OldTag);
	}
}

void AGSGameState::AddPhaseTag(FGameplayTag PhaseTag)
{
	if (HasAuthority() && PhaseTag.IsValid() && !ActivePhaseTags.HasTagExact(PhaseTag))
	{
		ActivePhaseTags.AddTag(PhaseTag);
		OnPhaseTagAddedRemoved.Broadcast(PhaseTag, true);
	}
}

void AGSGameState::RemovePhaseTag(FGameplayTag PhaseTag)
{
	if (HasAuthority() && PhaseTag.IsValid() && ActivePhaseTags.HasTagExact(PhaseTag))
	{
		ActivePhaseTags.RemoveTag(PhaseTag);
		OnPhaseTagAddedRemoved.Broadcast(PhaseTag, false);
	}
}

bool AGSGameState::HasPhaseTag(FGameplayTag PhaseTag) const
{
	return ActivePhaseTags.HasTagExact(PhaseTag) || (PrimaryPhaseTag.IsValid() && PrimaryPhaseTag.MatchesTag(PhaseTag));
}

void AGSGameState::SetRemainingTime(float NewTime)
{
	if (HasAuthority())
	{
		RemainingTime = FMath::Max(NewTime, 0.0f);
		OnRemainingTimeChanged.Broadcast(RemainingTime);
	}
}

void AGSGameState::AddRemainingTime(float TimeToAdd)
{
	if (HasAuthority() && TimeToAdd != 0.0f)
	{
		SetRemainingTime(RemainingTime + TimeToAdd);
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
		}
	}
}

void AGSGameState::RegisterNPCManager(AGSNPCManager* Manager)
{
	NPCManager = Manager;
	

	if (HasAuthority() && NPCManager && CurrentPhase == EGSGamePhase::RoundInProgress)
	{
		NPCManager->StartSpawning();
	}
}

void AGSGameState::OnRep_CurrentPhase(EGSGamePhase OldPhase)
{
	OnGamePhaseChanged.Broadcast(CurrentPhase);
}

void AGSGameState::OnRep_PrimaryPhaseTag(FGameplayTag OldTag)
{
	OnPrimaryPhaseTagChanged.Broadcast(PrimaryPhaseTag);
}

void AGSGameState::OnRep_ActivePhaseTags(FGameplayTagContainer OldTags)
{
}

void AGSGameState::OnRep_RemainingTime(float OldTime)
{
	OnRemainingTimeChanged.Broadcast(RemainingTime);
}
