#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSFillAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTF_API UGSFillAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSFillAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_FillProgress)
	FGameplayAttributeData FillProgress;
	ATTRIBUTE_ACCESSORS(UGSFillAttributeSet, FillProgress)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxFillProgress)
	FGameplayAttributeData MaxFillProgress;
	ATTRIBUTE_ACCESSORS(UGSFillAttributeSet, MaxFillProgress)

protected:
	UFUNCTION()
	virtual void OnRep_FillProgress(const FGameplayAttributeData& OldFillProgress);

	UFUNCTION()
	virtual void OnRep_MaxFillProgress(const FGameplayAttributeData& OldMaxFillProgress);
};
