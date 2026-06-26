#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSPatienceAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTF_API UGSPatienceAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSPatienceAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Patience)
	FGameplayAttributeData Patience;
	ATTRIBUTE_ACCESSORS(UGSPatienceAttributeSet, Patience)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxPatience)
	FGameplayAttributeData MaxPatience;
	ATTRIBUTE_ACCESSORS(UGSPatienceAttributeSet, MaxPatience)

protected:
	UFUNCTION()
	virtual void OnRep_Patience(const FGameplayAttributeData& OldPatience);

	UFUNCTION()
	virtual void OnRep_MaxPatience(const FGameplayAttributeData& OldMaxPatience);
};
