#pragma once

#include "CoreMinimal.h"
#include "UI/GSFloatingWidgetBase.h"
#include "DataAssets/GSMenuDataAsset.h"
#include "GSNPCOrderWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;

/**
 * Specialized Widget class for NPC Food Order UI floating over the NPC.
 * Inherits from UGSFloatingWidgetBase for modular functionality.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTF_API UGSNPCOrderWidget : public UGSFloatingWidgetBase
{
	GENERATED_BODY()

public:
	UGSNPCOrderWidget(const FObjectInitializer& ObjectInitializer);

	/** Backward compatible binding for Food Icon Image */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Order UI")
	TObjectPtr<UImage> FoodIconImage;

	/** Backward compatible binding for Food Name Text */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Order UI")
	TObjectPtr<UTextBlock> FoodNameText;

	/** Backward compatible binding for Patience Progress Bar */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Order UI")
	TObjectPtr<UProgressBar> PatienceProgressBar;

	/** Current order details assigned to this widget */
	UPROPERTY(BlueprintReadOnly, Category = "Order UI")
	FGSFoodRecipeDetails CurrentOrder;

	/** Updates the UI elements with the provided food recipe details */
	UFUNCTION(BlueprintCallable, Category = "Order UI")
	virtual void SetOrderDetails(const FGSFoodRecipeDetails& Recipe);

	/** Updates the patience progress bar (0.0 to 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Order UI")
	virtual void SetPatiencePercent(float NormalizedPatience);

	/** Blueprint Event triggered whenever order details are updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "Order UI")
	void BP_OnOrderUpdated(const FGSFoodRecipeDetails& Recipe);
};
