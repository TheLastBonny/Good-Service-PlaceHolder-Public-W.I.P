#pragma once

#include "CoreMinimal.h"
#include "Items/GSItem.h"
#include "GSMoneyItem.generated.h"

class UGameplayEffect;

UCLASS()
class PROJECTF_API AGSMoneyItem : public AGSItem
{
	GENERATED_BODY()

public:
	AGSMoneyItem();

	// Value of this money drop
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
	float MoneyValue;

	// Gameplay Effect applied to the GameState's ASC to award money (modifies EarnedMoney using SetByCaller Data.MoneyAmount)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> MoneyEffectClass;
};
