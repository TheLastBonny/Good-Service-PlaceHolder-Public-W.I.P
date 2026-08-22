#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "GSPauseMenuWidget.generated.h"

USTRUCT(BlueprintType)
struct FGSPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Player Info")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Player Info")
	int32 PlayerId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Player Info")
	int32 Ping = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Player Info")
	bool bIsLocalPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player Info")
	bool bIsReady = false;
};

/**
 * Base C++ Widget class for the Multiplayer Pause Menu.
 * Handles audio settings (Master, Music, SFX), player list (1-4 players), and menu navigation.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTF_API UGSPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGSPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	// --- AUDIO SETTINGS ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundMix> MainSoundMix;

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetSFXVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetCustomSoundClassVolume(USoundClass* TargetSoundClass, float Volume);

	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetSFXVolume() const { return SFXVolume; }

	// --- PLAYER LIST (1-4 PLAYERS) ---

	/** Queries the GameState for connected players and returns an array of player info */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	TArray<FGSPlayerInfo> FetchPlayerList();

	/** Event triggered when the player list is refreshed (override in BP or bind to rebuild UI slots) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Multiplayer")
	void BP_OnPlayerListRefreshed(const TArray<FGSPlayerInfo>& Players);

	// --- MENU ACTIONS ---

	/** Resumes the game (hides menu and restores input mode) */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void ResumeGame();

	/** Quits the current multiplayer session and leaves to main menu or desktop */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void QuitGame();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	float MusicVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	float SFXVolume = 1.0f;

	void ApplySoundClassVolume(USoundClass* SoundClass, float Volume);
	void LoadAudioSettings();
	void SaveAudioSettings();
};
