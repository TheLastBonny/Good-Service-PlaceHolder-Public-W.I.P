#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSBurnAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTF_API UGSBurnAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSBurnAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_BurnProgress)
	FGameplayAttributeData BurnProgress;
	ATTRIBUTE_ACCESSORS(UGSBurnAttributeSet, BurnProgress)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxBurnProgress)
	FGameplayAttributeData MaxBurnProgress;
	ATTRIBUTE_ACCESSORS(UGSBurnAttributeSet, MaxBurnProgress)

protected:
	UFUNCTION()
	virtual void OnRep_BurnProgress(const FGameplayAttributeData& OldBurnProgress);

	UFUNCTION()
	virtual void OnRep_MaxBurnProgress(const FGameplayAttributeData& OldMaxBurnProgress);
};
