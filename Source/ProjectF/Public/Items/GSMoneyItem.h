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


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
	float MoneyValue;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> MoneyEffectClass;
};
