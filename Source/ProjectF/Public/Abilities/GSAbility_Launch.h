#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GSAbility_Launch.generated.h"

UCLASS()
class PROJECTF_API UGSAbility_Launch : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGSAbility_Launch();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnInputReleased(float HeldTime);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Launch")
	float HoldThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Launch")
	float BaseThrowForce;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Launch")
	float MinThrowDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Launch")
	float MaxThrowDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Launch")
	float DropForwardOffset;

private:
	float ActivationTime;
};
