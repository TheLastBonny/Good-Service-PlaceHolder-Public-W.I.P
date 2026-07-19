#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GSEmoteDefinition.generated.h"

class UTexture2D;
class UAnimMontage;
class USoundBase;

/**
 * Data asset defining an individual emote.
 */
UCLASS(BlueprintType)
class PROJECTF_API UGSEmoteDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote")
	FText EmoteName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote")
	FText EmoteDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote")
	TSoftObjectPtr<UTexture2D> EmoteIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote")
	TSoftObjectPtr<UAnimMontage> EmoteMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote")
	TSoftObjectPtr<USoundBase> EmoteSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote")
	FGameplayTag EmoteTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote")
	bool bLooping = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote | Sound")
	bool bPlayAs3DSound = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote | Sound", meta = (EditCondition = "bPlayAs3DSound"))
	float SoundRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote | Sound", meta = (EditCondition = "bPlayAs3DSound"))
	bool bShowSoundRadiusDebug = false;
};

/**
 * Data asset containing a list of emotes.
 */
UCLASS(BlueprintType)
class PROJECTF_API UGSEmoteLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emotes")
	TArray<TObjectPtr<UGSEmoteDefinition>> Emotes;
};
