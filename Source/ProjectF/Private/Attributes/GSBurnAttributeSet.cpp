#include "Attributes/GSBurnAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameplayTags.h"

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
		NewValue = FMath::Max(NewValue, 0.f);
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
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			const FGameplayTag BurnedTag = GSGameplayTags::State_Burned;
			if (NewValue >= GetMaxBurnProgress())
			{
				if (!ASC->HasMatchingGameplayTag(BurnedTag))
				{
					ASC->AddLooseGameplayTag(BurnedTag);
					ASC->SetLooseGameplayTagCount(BurnedTag, 1);
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
