#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GSAbility_PlayEmote.generated.h"

UCLASS()
class PROJECTF_API UGSAbility_PlayEmote : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGSAbility_PlayEmote();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnEmoteTimerFinished();

private:
	FTimerHandle EmoteTimerHandle;
};
