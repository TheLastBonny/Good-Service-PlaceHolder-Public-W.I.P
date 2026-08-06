#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GSAbilityUpgradeDataAsset.generated.h"

UENUM(BlueprintType)
enum class EGSAUpgradeCostType : uint8
{
	Money    UMETA(DisplayName = "Money"),
	Object   UMETA(DisplayName = "Object Tag")
};

USTRUCT(BlueprintType)
struct FGSAbilityUpgradeLevel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	int32 TargetLevel = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	EGSAUpgradeCostType CostType = EGSAUpgradeCostType::Money;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (EditCondition = "CostType == EGSAUpgradeCostType::Money"))
	int32 MoneyCost = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (EditCondition = "CostType == EGSAUpgradeCostType::Object"))
	FGameplayTag RequiredObjectTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (EditCondition = "CostType == EGSAUpgradeCostType::Object"))
	int32 RequiredQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FText UpgradeDescription;
};

UCLASS(BlueprintType)
class PROJECTF_API UGSAbilityUpgradeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSAbilityUpgradeDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TArray<FGSAbilityUpgradeLevel> UpgradeLevels;

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	bool GetUpgradeForLevel(int32 TargetLevel, FGSAbilityUpgradeLevel& OutUpgrade) const;

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetMaxLevel() const;
};
