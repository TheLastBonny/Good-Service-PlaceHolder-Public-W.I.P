#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GSItem.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UGSItemDataAsset;

UCLASS()
class PROJECTF_API AGSItem : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSItem();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void OnRep_AttachmentReplication() override;
	virtual void OnRep_ReplicatedMovement() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugLogs = false;

	UFUNCTION(CallInEditor, Category = "Debug")
	void ToggleDebugLogs() { bShowDebugLogs = !bShowDebugLogs; }

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	FGameplayTagContainer ItemTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGSItemDataAsset> ItemData;

	virtual void OnStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
};
