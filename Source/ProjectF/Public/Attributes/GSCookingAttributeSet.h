#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSCookingAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTF_API UGSCookingAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSCookingAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CookingProgress)
	FGameplayAttributeData CookingProgress;
	ATTRIBUTE_ACCESSORS(UGSCookingAttributeSet, CookingProgress)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxCookingProgress)
	FGameplayAttributeData MaxCookingProgress;
	ATTRIBUTE_ACCESSORS(UGSCookingAttributeSet, MaxCookingProgress)

protected:
	UFUNCTION()
	virtual void OnRep_CookingProgress(const FGameplayAttributeData& OldCookingProgress);

	UFUNCTION()
	virtual void OnRep_MaxCookingProgress(const FGameplayAttributeData& OldMaxCookingProgress);
};
