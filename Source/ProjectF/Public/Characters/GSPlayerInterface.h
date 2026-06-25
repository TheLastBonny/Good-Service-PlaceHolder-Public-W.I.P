#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "GSPlayerInterface.generated.h"

UINTERFACE(MinimalAPI)
class UGSPlayerInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTF_API IGSPlayerInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Player")
	void RequestMove(const FVector2D& MovementVector);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Player")
	void RequestJump(bool bIsJumping);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Player")
	void RequestAbilityByTag(const FGameplayTag& InputTag);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Player")
	void RequestAbilityReleasedByTag(const FGameplayTag& InputTag);
};
