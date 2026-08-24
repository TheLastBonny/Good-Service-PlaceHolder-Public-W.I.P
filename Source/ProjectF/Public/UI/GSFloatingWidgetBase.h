#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSFloatingWidgetBase.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;

/**
 * Modular base class for any floating 3D/2D UI Widget (Orders, Dialogues, Emotes, Statuses, Quests).
 * Uses optional widget bindings so UI designers are free to build custom layouts in UMG.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTF_API UGSFloatingWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UGSFloatingWidgetBase(const FObjectInitializer& ObjectInitializer);

	/** Optional icon image widget */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Floating UI")
	TObjectPtr<UImage> IconImage;

	/** Optional main title text widget */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Floating UI")
	TObjectPtr<UTextBlock> TitleText;

	/** Optional subtitle or description text widget */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Floating UI")
	TObjectPtr<UTextBlock> DescriptionText;

	/** Optional progress or patience bar widget */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Floating UI")
	TObjectPtr<UProgressBar> ProgressBar;

	/** Updates the icon image asynchronously/synchronously */
	UFUNCTION(BlueprintCallable, Category = "Floating UI")
	virtual void SetIcon(TSoftObjectPtr<UTexture2D> SoftIcon);

	/** Updates main title text */
	UFUNCTION(BlueprintCallable, Category = "Floating UI")
	virtual void SetTitle(const FText& InTitle);

	/** Updates description text */
	UFUNCTION(BlueprintCallable, Category = "Floating UI")
	virtual void SetDescription(const FText& InDescription);

	/** Updates progress bar (0.0 to 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Floating UI")
	virtual void SetProgress(float Percent);

	/** Sets all floating UI data in one call */
	UFUNCTION(BlueprintCallable, Category = "Floating UI")
	virtual void SetWidgetData(TSoftObjectPtr<UTexture2D> SoftIcon, const FText& InTitle, const FText& InDescription, float Percent);

	/** Event triggered whenever widget data is updated for Blueprint animations/audio */
	UFUNCTION(BlueprintImplementableEvent, Category = "Floating UI")
	void BP_OnWidgetDataUpdated();
};
