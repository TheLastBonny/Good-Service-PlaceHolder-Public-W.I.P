#include "UI/GSFloatingWidgetBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"

UGSFloatingWidgetBase::UGSFloatingWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGSFloatingWidgetBase::SetIcon(TSoftObjectPtr<UTexture2D> SoftIcon)
{
	if (!IconImage) return;

	if (!SoftIcon.IsNull())
	{
		UTexture2D* LoadedTexture = SoftIcon.Get();
		if (!LoadedTexture)
		{
			LoadedTexture = SoftIcon.LoadSynchronous();
		}

		if (LoadedTexture)
		{
			IconImage->SetBrushFromTexture(LoadedTexture);
			IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGSFloatingWidgetBase::SetTitle(const FText& InTitle)
{
	if (TitleText)
	{
		TitleText->SetText(InTitle);
	}
}

void UGSFloatingWidgetBase::SetDescription(const FText& InDescription)
{
	if (DescriptionText)
	{
		DescriptionText->SetText(InDescription);
	}
}

void UGSFloatingWidgetBase::SetProgress(float Percent)
{
	if (ProgressBar)
	{
		ProgressBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UGSFloatingWidgetBase::SetWidgetData(TSoftObjectPtr<UTexture2D> SoftIcon, const FText& InTitle, const FText& InDescription, float Percent)
{
	SetIcon(SoftIcon);
	SetTitle(InTitle);
	SetDescription(InDescription);
	SetProgress(Percent);

	BP_OnWidgetDataUpdated();
}
