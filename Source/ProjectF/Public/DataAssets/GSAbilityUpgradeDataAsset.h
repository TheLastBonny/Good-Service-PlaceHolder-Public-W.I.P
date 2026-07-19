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

	/** The level that this upgrade will grant (e.g., Level 2, Level 3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	int32 TargetLevel = 2;

	/** The type of cost required for this upgrade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	EGSAUpgradeCostType CostType = EGSAUpgradeCostType::Money;

	/** If CostType is Money, this is the amount of money deducted from GameState. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (EditCondition = "CostType == EGSAUpgradeCostType::Money"))
	int32 MoneyCost = 100;

	/** If CostType is Object, this is the tag detected on the overlapping object(s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (EditCondition = "CostType == EGSAUpgradeCostType::Object"))
	FGameplayTag RequiredObjectTag;

	/** If CostType is Object, this is how many matching objects must be present in the trigger volume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (EditCondition = "CostType == EGSAUpgradeCostType::Object"))
	int32 RequiredQuantity = 1;

	/** Optional description of what this level upgrade does. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FText UpgradeDescription;
};

UCLASS(BlueprintType)
class PROJECTF_API UGSAbilityUpgradeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSAbilityUpgradeDataAsset();

	/** Configuration of cost and requirements per upgrade level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TArray<FGSAbilityUpgradeLevel> UpgradeLevels;

	/** Finds the upgrade configuration for a specific target level. */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	bool GetUpgradeForLevel(int32 TargetLevel, FGSAbilityUpgradeLevel& OutUpgrade) const;

	/** Gets the maximum level defined in this data asset. */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetMaxLevel() const;
};
