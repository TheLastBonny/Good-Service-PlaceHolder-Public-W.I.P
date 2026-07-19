#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSMoneyValueComponent.generated.h"

class UGameplayEffect;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTF_API UGSMoneyValueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSMoneyValueComponent();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
	float MoneyValue;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
	TSubclassOf<UGameplayEffect> MoneyEffectClass;
};
