#include "Attributes/GSCookingAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameplayTags.h"

TAutoConsoleVariable<int32> CVarShowDebugLogs(
	TEXT("gs.ShowDebugLogs"),
	0,
	TEXT("Toggle GoodService debug logs project-wide.\n0: Off\n1: On"),
	ECVF_Cheat
);

UGSCookingAttributeSet::UGSCookingAttributeSet()
{
	InitCookingProgress(0.f);
	InitMaxCookingProgress(100.f);
}

void UGSCookingAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSCookingAttributeSet, CookingProgress, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSCookingAttributeSet, MaxCookingProgress, COND_None, REPNOTIFY_Always);
}

void UGSCookingAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCookingProgressAttribute())
	{

		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxCookingProgress());
	}
	else if (Attribute == GetMaxCookingProgressAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGSCookingAttributeSet::OnRep_CookingProgress(const FGameplayAttributeData& OldCookingProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCookingAttributeSet, CookingProgress, OldCookingProgress);
}

void UGSCookingAttributeSet::OnRep_MaxCookingProgress(const FGameplayAttributeData& OldMaxCookingProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCookingAttributeSet, MaxCookingProgress, OldMaxCookingProgress);
}

void UGSCookingAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetCookingProgressAttribute())
	{
		if (CVarShowDebugLogs.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[COOKING] %s: CookingProgress = %f / %f"), *GetOwningActor()->GetName(), NewValue, GetMaxCookingProgress());
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					(int32)((uintptr_t)this),
					0.1f,
					FColor::Yellow,
					FString::Printf(TEXT("%s: Cooking Progress = %.1f / %.1f"), *GetOwningActor()->GetName(), NewValue, GetMaxCookingProgress())
				);
			}
		}

		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			const FGameplayTag CookedTag = GSGameplayTags::State_Cooked;
			if (NewValue >= GetMaxCookingProgress())
			{
				if (!ASC->HasMatchingGameplayTag(CookedTag))
				{

					ASC->AddLooseGameplayTag(CookedTag, 1, EGameplayTagReplicationState::TagOnly);

					ASC->RemoveLooseGameplayTag(GSGameplayTags::State_Raw, 1, EGameplayTagReplicationState::TagOnly);
				}
			}
			else
			{
				if (ASC->HasMatchingGameplayTag(CookedTag))
				{
					ASC->RemoveLooseGameplayTag(CookedTag);
				}
			}
		}
	}
}
