#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSStoveAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTF_API UGSStoveAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSStoveAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_HeatLevel)
	FGameplayAttributeData HeatLevel;
	ATTRIBUTE_ACCESSORS(UGSStoveAttributeSet, HeatLevel)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CookingSpeed)
	FGameplayAttributeData CookingSpeed;
	ATTRIBUTE_ACCESSORS(UGSStoveAttributeSet, CookingSpeed)

protected:
	UFUNCTION()
	virtual void OnRep_HeatLevel(const FGameplayAttributeData& OldHeatLevel);

	UFUNCTION()
	virtual void OnRep_CookingSpeed(const FGameplayAttributeData& OldCookingSpeed);
};
