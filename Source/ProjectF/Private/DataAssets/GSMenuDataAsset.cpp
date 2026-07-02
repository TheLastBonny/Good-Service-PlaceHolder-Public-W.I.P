#include "DataAssets/GSMenuDataAsset.h"

UGSMenuDataAsset::UGSMenuDataAsset()
{
}

FGSFoodRecipeDetails UGSMenuDataAsset::GetRandomRecipe() const
{
	if (AvailableRecipes.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, AvailableRecipes.Num() - 1);
		return AvailableRecipes[RandomIndex];
	}
	return FGSFoodRecipeDetails();
}
