#include "Attributes/GSCoolingAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameplayTags.h"

UGSCoolingAttributeSet::UGSCoolingAttributeSet()
{
	InitCoolingProgress(0.f);
	InitMaxCoolingProgress(100.f);
}

void UGSCoolingAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSCoolingAttributeSet, CoolingProgress, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSCoolingAttributeSet, MaxCoolingProgress, COND_None, REPNOTIFY_Always);
}

void UGSCoolingAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCoolingProgressAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetMaxCoolingProgressAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGSCoolingAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetCoolingProgressAttribute())
	{
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			const FGameplayTag CooledTag = GSGameplayTags::State_Cooled;
			const float MaxProgress = GetMaxCoolingProgress();
			if (MaxProgress > 0.f && NewValue >= MaxProgress)
			{
				if (!ASC->HasMatchingGameplayTag(CooledTag))
				{
					ASC->AddLooseGameplayTag(CooledTag);
					ASC->SetLooseGameplayTagCount(CooledTag, 1);
				}
			}
			else
			{
				if (ASC->HasMatchingGameplayTag(CooledTag))
				{
					ASC->RemoveLooseGameplayTag(CooledTag);
				}
			}
		}
	}
}

void UGSCoolingAttributeSet::OnRep_CoolingProgress(const FGameplayAttributeData& OldCoolingProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCoolingAttributeSet, CoolingProgress, OldCoolingProgress);
}

void UGSCoolingAttributeSet::OnRep_MaxCoolingProgress(const FGameplayAttributeData& OldMaxCoolingProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCoolingAttributeSet, MaxCoolingProgress, OldMaxCoolingProgress);
}
