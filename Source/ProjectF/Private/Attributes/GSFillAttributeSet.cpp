#include "Attributes/GSFillAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameplayTags.h"

UGSFillAttributeSet::UGSFillAttributeSet()
{
	InitFillProgress(0.f);
	InitMaxFillProgress(100.f);
}

void UGSFillAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSFillAttributeSet, FillProgress, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSFillAttributeSet, MaxFillProgress, COND_None, REPNOTIFY_Always);
}

void UGSFillAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetFillProgressAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetMaxFillProgressAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGSFillAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetFillProgressAttribute())
	{
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			const FGameplayTag FilledTag = GSGameplayTags::State_Filled;
			if (NewValue >= GetMaxFillProgress())
			{
				if (!ASC->HasMatchingGameplayTag(FilledTag))
				{
					ASC->AddLooseGameplayTag(FilledTag);
					ASC->SetLooseGameplayTagCount(FilledTag, 1);
				}
			}
			else
			{
				if (ASC->HasMatchingGameplayTag(FilledTag))
				{
					ASC->RemoveLooseGameplayTag(FilledTag);
				}
			}
		}
	}
}

void UGSFillAttributeSet::OnRep_FillProgress(const FGameplayAttributeData& OldFillProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSFillAttributeSet, FillProgress, OldFillProgress);
}

void UGSFillAttributeSet::OnRep_MaxFillProgress(const FGameplayAttributeData& OldMaxFillProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSFillAttributeSet, MaxFillProgress, OldMaxFillProgress);
}
