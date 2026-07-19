#include "Attributes/GSMoneyAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Core/GSGameState.h"

UGSMoneyAttributeSet::UGSMoneyAttributeSet()
{
	InitMoney(0.f);
	InitMoneyMultiplier(1.f);
	InitEarnedMoney(0.f);
}

void UGSMoneyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSMoneyAttributeSet, Money, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSMoneyAttributeSet, MoneyMultiplier, COND_None, REPNOTIFY_Always);
}

void UGSMoneyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMoneyAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetMoneyMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGSMoneyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetEarnedMoneyAttribute())
	{
		const float LocalEarned = GetEarnedMoney();
		SetEarnedMoney(0.f);

		if (LocalEarned > 0.f)
		{
			const float MultipliedEarned = LocalEarned * GetMoneyMultiplier();
			const float NewMoney = GetMoney() + MultipliedEarned;
			SetMoney(FMath::Max(NewMoney, 0.f));


			if (AActor* OwningActor = GetOwningActor())
			{
				if (AGSGameState* GSGameState = Cast<AGSGameState>(OwningActor))
				{
					GSGameState->OnMoneyChanged.Broadcast(FMath::RoundToInt(GetMoney()));
				}
			}
		}
	}
}

void UGSMoneyAttributeSet::OnRep_Money(const FGameplayAttributeData& OldMoney)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSMoneyAttributeSet, Money, OldMoney);
	
	if (AActor* OwningActor = GetOwningActor())
	{
		if (AGSGameState* GSGameState = Cast<AGSGameState>(OwningActor))
		{
			GSGameState->OnMoneyChanged.Broadcast(FMath::RoundToInt(GetMoney()));
		}
	}
}

void UGSMoneyAttributeSet::OnRep_MoneyMultiplier(const FGameplayAttributeData& OldMoneyMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSMoneyAttributeSet, MoneyMultiplier, OldMoneyMultiplier);

	if (AActor* OwningActor = GetOwningActor())
	{
		if (AGSGameState* GSGameState = Cast<AGSGameState>(OwningActor))
		{
			GSGameState->OnMoneyMultiplierChanged.Broadcast(GetMoneyMultiplier());
		}
	}
}
