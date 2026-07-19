#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSMoneyAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTF_API UGSMoneyAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSMoneyAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;


	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Money)
	FGameplayAttributeData Money;
	ATTRIBUTE_ACCESSORS(UGSMoneyAttributeSet, Money)


	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoneyMultiplier)
	FGameplayAttributeData MoneyMultiplier;
	ATTRIBUTE_ACCESSORS(UGSMoneyAttributeSet, MoneyMultiplier)


	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData EarnedMoney;
	ATTRIBUTE_ACCESSORS(UGSMoneyAttributeSet, EarnedMoney)

protected:
	UFUNCTION()
	virtual void OnRep_Money(const FGameplayAttributeData& OldMoney);

	UFUNCTION()
	virtual void OnRep_MoneyMultiplier(const FGameplayAttributeData& OldMoneyMultiplier);
};
