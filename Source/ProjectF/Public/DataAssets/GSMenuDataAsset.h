#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GSMenuDataAsset.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FGSFoodRecipeDetails
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FGameplayTag FoodTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FText FoodName;

	// Soft pointer to icon texture so we can load it on demand
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TSoftObjectPtr<UTexture2D> FoodIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	float BasePrice = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	float BasePatienceMultiplier = 1.0f;
};

UCLASS(BlueprintType)
class PROJECTF_API UGSMenuDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSMenuDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	TArray<FGSFoodRecipeDetails> AvailableRecipes;

	UFUNCTION(BlueprintPure, Category = "Menu")
	FGSFoodRecipeDetails GetRandomRecipe() const;
};
