#include "Attributes/GSPatienceAttributeSet.h"
#include "Net/UnrealNetwork.h"

UGSPatienceAttributeSet::UGSPatienceAttributeSet()
{
	InitPatience(100.f);
	InitMaxPatience(100.f);
}

void UGSPatienceAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSPatienceAttributeSet, Patience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSPatienceAttributeSet, MaxPatience, COND_None, REPNOTIFY_Always);
}

void UGSPatienceAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetPatienceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPatience());
	}
	else if (Attribute == GetMaxPatienceAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGSPatienceAttributeSet::OnRep_Patience(const FGameplayAttributeData& OldPatience)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSPatienceAttributeSet, Patience, OldPatience);
}

void UGSPatienceAttributeSet::OnRep_MaxPatience(const FGameplayAttributeData& OldMaxPatience)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSPatienceAttributeSet, MaxPatience, OldMaxPatience);
}
