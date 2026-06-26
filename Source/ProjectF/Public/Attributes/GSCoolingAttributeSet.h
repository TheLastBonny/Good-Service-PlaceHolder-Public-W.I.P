#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSCoolingAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTF_API UGSCoolingAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSCoolingAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CoolingProgress)
	FGameplayAttributeData CoolingProgress;
	ATTRIBUTE_ACCESSORS(UGSCoolingAttributeSet, CoolingProgress)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxCoolingProgress)
	FGameplayAttributeData MaxCoolingProgress;
	ATTRIBUTE_ACCESSORS(UGSCoolingAttributeSet, MaxCoolingProgress)

protected:
	UFUNCTION()
	virtual void OnRep_CoolingProgress(const FGameplayAttributeData& OldCoolingProgress);

	UFUNCTION()
	virtual void OnRep_MaxCoolingProgress(const FGameplayAttributeData& OldMaxCoolingProgress);
};
