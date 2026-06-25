#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GSInputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FGSInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag InputTag;
};

UCLASS(BlueprintType)
class PROJECTF_API UGSInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TArray<FGSInputAction> AbilityInputActions;

	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
};
