#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GSAbility_Grab.generated.h"

UCLASS()
class PROJECTF_API UGSAbility_Grab : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGSAbility_Grab();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	float GrabRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	float GrabForwardOffset;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bShowDebugShape;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	FColor DebugColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	float DebugLifeTime;
};
