#include "Attributes/GSBurnAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameplayTags.h"

extern TAutoConsoleVariable<int32> CVarShowDebugLogs;

UGSBurnAttributeSet::UGSBurnAttributeSet()
{
	InitBurnProgress(0.f);
	InitMaxBurnProgress(100.f);
}

void UGSBurnAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSBurnAttributeSet, BurnProgress, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSBurnAttributeSet, MaxBurnProgress, COND_None, REPNOTIFY_Always);
}

void UGSBurnAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetBurnProgressAttribute())
	{

		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxBurnProgress());
	}
	else if (Attribute == GetMaxBurnProgressAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGSBurnAttributeSet::OnRep_BurnProgress(const FGameplayAttributeData& OldBurnProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSBurnAttributeSet, BurnProgress, OldBurnProgress);
}

void UGSBurnAttributeSet::OnRep_MaxBurnProgress(const FGameplayAttributeData& OldMaxBurnProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSBurnAttributeSet, MaxBurnProgress, OldMaxBurnProgress);
}

void UGSBurnAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetBurnProgressAttribute())
	{
		if (CVarShowDebugLogs.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[BURN] %s: BurnProgress = %f / %f"), *GetOwningActor()->GetName(), NewValue, GetMaxBurnProgress());
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					(int32)((uintptr_t)this),
					0.1f,
					FColor::Red,
					FString::Printf(TEXT("%s: Burning Progress = %.1f / %.1f"), *GetOwningActor()->GetName(), NewValue, GetMaxBurnProgress())
				);
			}
		}

		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			const FGameplayTag BurnedTag = GSGameplayTags::State_Burned;
			const float MaxProgress = GetMaxBurnProgress();
			if (MaxProgress > 0.f && NewValue >= MaxProgress)
			{
				if (!ASC->HasMatchingGameplayTag(BurnedTag))
				{
					ASC->AddLooseGameplayTag(BurnedTag, 1, EGameplayTagReplicationState::TagOnly);
					ASC->RemoveLooseGameplayTag(GSGameplayTags::State_Cooked, 1, EGameplayTagReplicationState::TagOnly);
				}
			}
			else
			{
				if (ASC->HasMatchingGameplayTag(BurnedTag))
				{
					ASC->RemoveLooseGameplayTag(BurnedTag);
				}
			}
		}
	}
}
