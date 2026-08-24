#include "DataAssets/GSAbilityUpgradeDataAsset.h"

UGSAbilityUpgradeDataAsset::UGSAbilityUpgradeDataAsset()
{
}

bool UGSAbilityUpgradeDataAsset::GetUpgradeForLevel(int32 TargetLevel, FGSAbilityUpgradeLevel& OutUpgrade) const
{
	for (const FGSAbilityUpgradeLevel& LevelInfo : UpgradeLevels)
	{
		if (LevelInfo.TargetLevel == TargetLevel)
		{
			OutUpgrade = LevelInfo;
			return true;
		}
	}
	return false;
}

int32 UGSAbilityUpgradeDataAsset::GetMaxLevel() const
{
	int32 MaxLevel = 1; // Base level is 1
	for (const FGSAbilityUpgradeLevel& LevelInfo : UpgradeLevels)
	{
		if (LevelInfo.TargetLevel > MaxLevel)
		{
			MaxLevel = LevelInfo.TargetLevel;
		}
	}
	return MaxLevel;
}
