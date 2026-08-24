#include "UI/GSNPCOrderWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

UGSNPCOrderWidget::UGSNPCOrderWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGSNPCOrderWidget::SetOrderDetails(const FGSFoodRecipeDetails& Recipe)
{
	CurrentOrder = Recipe;

	// Update base class bindings
	SetIcon(Recipe.FoodIcon);
	SetTitle(Recipe.FoodName);

	// Update backward compatible bindings if explicitly named FoodIconImage / FoodNameText
	if (FoodNameText && FoodNameText != TitleText)
	{
		FoodNameText->SetText(Recipe.FoodName);
	}

	if (FoodIconImage && FoodIconImage != IconImage)
	{
		if (!Recipe.FoodIcon.IsNull())
		{
			UTexture2D* LoadedTexture = Recipe.FoodIcon.Get();
			if (!LoadedTexture)
			{
				LoadedTexture = Recipe.FoodIcon.LoadSynchronous();
			}

			if (LoadedTexture)
			{
				FoodIconImage->SetBrushFromTexture(LoadedTexture);
				FoodIconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				FoodIconImage->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			FoodIconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	BP_OnOrderUpdated(Recipe);
}

void UGSNPCOrderWidget::SetPatiencePercent(float NormalizedPatience)
{
	SetProgress(NormalizedPatience);

	if (PatienceProgressBar && PatienceProgressBar != ProgressBar)
	{
		PatienceProgressBar->SetPercent(FMath::Clamp(NormalizedPatience, 0.0f, 1.0f));
	}
}
