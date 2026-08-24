#include "UI/GSPauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Characters/GSPlayerState.h"
#include "Characters/GSPlayerController.h"
#include "GameFramework/GameUserSettings.h"

UGSPauseMenuWidget::UGSPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MasterVolume = 1.0f;
	MusicVolume = 1.0f;
	SFXVolume = 1.0f;
}

void UGSPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LoadAudioSettings();
	TArray<FGSPlayerInfo> Players = FetchPlayerList();
	BP_OnPlayerListRefreshed(Players);
}

void UGSPauseMenuWidget::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplySoundClassVolume(MasterSoundClass, MasterVolume);
	SaveAudioSettings();
}

void UGSPauseMenuWidget::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplySoundClassVolume(MusicSoundClass, MusicVolume);
	SaveAudioSettings();
}

void UGSPauseMenuWidget::SetSFXVolume(float Volume)
{
	SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplySoundClassVolume(SFXSoundClass, SFXVolume);
	SaveAudioSettings();
}

void UGSPauseMenuWidget::SetCustomSoundClassVolume(USoundClass* TargetSoundClass, float Volume)
{
	if (TargetSoundClass)
	{
		ApplySoundClassVolume(TargetSoundClass, FMath::Clamp(Volume, 0.0f, 1.0f));
	}
}

void UGSPauseMenuWidget::ApplySoundClassVolume(USoundClass* SoundClass, float Volume)
{
	if (!SoundClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World && MainSoundMix)
	{
		UGameplayStatics::SetSoundMixClassOverride(World, MainSoundMix, SoundClass, Volume, 1.0f, 0.0f, true);
		UGameplayStatics::PushSoundMixModifier(World, MainSoundMix);
	}
	else
	{
		SoundClass->Properties.Volume = Volume;
	}
}

void UGSPauseMenuWidget::LoadAudioSettings()
{
	if (GConfig)
	{
		GConfig->GetFloat(TEXT("AudioSettings"), TEXT("MasterVolume"), MasterVolume, GGameIni);
		GConfig->GetFloat(TEXT("AudioSettings"), TEXT("MusicVolume"), MusicVolume, GGameIni);
		GConfig->GetFloat(TEXT("AudioSettings"), TEXT("SFXVolume"), SFXVolume, GGameIni);
	}

	ApplySoundClassVolume(MasterSoundClass, MasterVolume);
	ApplySoundClassVolume(MusicSoundClass, MusicVolume);
	ApplySoundClassVolume(SFXSoundClass, SFXVolume);
}

void UGSPauseMenuWidget::SaveAudioSettings()
{
	if (GConfig)
	{
		GConfig->SetFloat(TEXT("AudioSettings"), TEXT("MasterVolume"), MasterVolume, GGameIni);
		GConfig->SetFloat(TEXT("AudioSettings"), TEXT("MusicVolume"), MusicVolume, GGameIni);
		GConfig->SetFloat(TEXT("AudioSettings"), TEXT("SFXVolume"), SFXVolume, GGameIni);
		GConfig->Flush(false, GGameIni);
	}
}

TArray<FGSPlayerInfo> UGSPauseMenuWidget::FetchPlayerList()
{
	TArray<FGSPlayerInfo> OutPlayers;

	UWorld* World = GetWorld();
	if (!World)
	{
		return OutPlayers;
	}

	AGameStateBase* GS = World->GetGameState();
	APlayerController* LocalPC = GetOwningPlayer();

	if (GS)
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (!PS)
			{
				continue;
			}

			FGSPlayerInfo Info;
			Info.PlayerName = PS->GetPlayerName();
			if (Info.PlayerName.IsEmpty())
			{
				Info.PlayerName = FString::Printf(TEXT("Player %d"), PS->GetPlayerId());
			}

			Info.PlayerId = PS->GetPlayerId();
			Info.Ping = FMath::RoundToInt(PS->GetPingInMilliseconds());
			Info.bIsLocalPlayer = (LocalPC && PS == LocalPC->PlayerState);

			if (AGSPlayerState* GSPS = Cast<AGSPlayerState>(PS))
			{
				Info.bIsReady = GSPS->bIsReady;
			}

			OutPlayers.Add(Info);
		}
	}

	return OutPlayers;
}

void UGSPauseMenuWidget::ResumeGame()
{
	AGSPlayerController* PC = Cast<AGSPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->TogglePauseMenu();
	}
	else if (APlayerController* GenPC = GetOwningPlayer())
	{
		RemoveFromParent();
		FInputModeGameOnly InputMode;
		GenPC->SetInputMode(InputMode);
		GenPC->bShowMouseCursor = false;
	}
}

void UGSPauseMenuWidget::QuitGame()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}
