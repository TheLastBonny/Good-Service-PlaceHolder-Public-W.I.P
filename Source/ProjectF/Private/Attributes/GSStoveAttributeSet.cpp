#include "Attributes/GSStoveAttributeSet.h"
#include "Net/UnrealNetwork.h"

UGSStoveAttributeSet::UGSStoveAttributeSet()
{
	InitHeatLevel(10.f);
	InitCookingSpeed(1.0f);
}

void UGSStoveAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSStoveAttributeSet, HeatLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSStoveAttributeSet, CookingSpeed, COND_None, REPNOTIFY_Always);
}

void UGSStoveAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHeatLevelAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetCookingSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGSStoveAttributeSet::OnRep_HeatLevel(const FGameplayAttributeData& OldHeatLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSStoveAttributeSet, HeatLevel, OldHeatLevel);
}

void UGSStoveAttributeSet::OnRep_CookingSpeed(const FGameplayAttributeData& OldCookingSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSStoveAttributeSet, CookingSpeed, OldCookingSpeed);
}
