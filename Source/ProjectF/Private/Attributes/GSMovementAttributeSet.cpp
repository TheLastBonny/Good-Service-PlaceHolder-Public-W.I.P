#include "Attributes/GSMovementAttributeSet.h"
#include "Net/UnrealNetwork.h"

UGSMovementAttributeSet::UGSMovementAttributeSet()
{
	InitWalkSpeed(800.f);
}

void UGSMovementAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSMovementAttributeSet, WalkSpeed, COND_None, REPNOTIFY_Always);
}

void UGSMovementAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetWalkSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGSMovementAttributeSet::OnRep_WalkSpeed(const FGameplayAttributeData& OldWalkSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSMovementAttributeSet, WalkSpeed, OldWalkSpeed);
}
