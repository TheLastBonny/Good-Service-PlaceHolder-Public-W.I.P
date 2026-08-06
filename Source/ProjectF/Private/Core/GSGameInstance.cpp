

#include "Core/GSGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"


#ifndef SEARCH_PRESENCE
#define SEARCH_PRESENCE FName(TEXT("PRESENCESEARCH"))
#endif

UGSGameInstance::UGSGameInstance()
{
	bIsHostPending = false;
	PendingMaxPlayers = 4;
}

void UGSGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGSGameInstance::OnCreateSessionComplete);
			CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

			DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UGSGameInstance::OnDestroySessionComplete);
			DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

			FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGSGameInstance::OnFindSessionsComplete);
			FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

			JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGSGameInstance::OnJoinSessionCompleteInternal);
			JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

			OnSessionUserInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
				FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UGSGameInstance::OnSessionUserInviteAccepted)
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGSGameInstance::Init: OnlineSubsystem is null"));
	}
}

void UGSGameInstance::Shutdown()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
			SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
		}
	}

	Super::Shutdown();
}

void UGSGameInstance::HostGame(const FString& RoomCode, int32 MaxPlayers)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("HostGame: Failed to get OnlineSubsystem"));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostGame: Failed to get SessionInterface"));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	if (RoomCode.IsEmpty())
	{

		const FString Alphanumeric = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
		FString GeneratedCode = TEXT("");
		for (int32 i = 0; i < 5; ++i)
		{
			int32 Index = FMath::RandRange(0, Alphanumeric.Len() - 1);
			GeneratedCode.AppendChar(Alphanumeric[Index]);
		}
		PendingRoomCode = GeneratedCode;
		
	}
	else
	{
		PendingRoomCode = RoomCode.ToUpper().TrimStartAndEnd();
	}
	PendingMaxPlayers = MaxPlayers;
	bIsHostPending = true;

	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("HostGame: Existing session found, destroying first..."));
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	bIsHostPending = false;

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.NumPublicConnections = PendingMaxPlayers;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowInvites = true;

	FOnlineSessionSetting RoomCodeSetting;
	RoomCodeSetting.Data = PendingRoomCode;
	RoomCodeSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
	SessionSettings.Settings.Add(FName(TEXT("ROOM_CODE")), RoomCodeSetting);

	bool bSuccess = SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("HostGame: Failed to initiate CreateSession call"));
		OnHostSessionComplete.Broadcast(false);
	}
}

void UGSGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete: %s, Successful: %d"), *SessionName.ToString(), bWasSuccessful);

	if (bIsHostPending)
	{
		bIsHostPending = false;
		
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
		if (Subsystem)
		{
			IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
			if (SessionInterface.IsValid())
			{
				FOnlineSessionSettings SessionSettings;
				SessionSettings.bIsLANMatch = false;
				SessionSettings.NumPublicConnections = PendingMaxPlayers;
				SessionSettings.bAllowJoinInProgress = true;
				SessionSettings.bAllowJoinViaPresence = true;
				SessionSettings.bUsesPresence = true;
				SessionSettings.bUseLobbiesIfAvailable = true;
				SessionSettings.bShouldAdvertise = true;
				SessionSettings.bAllowInvites = true;

				FOnlineSessionSetting RoomCodeSetting;
				RoomCodeSetting.Data = PendingRoomCode;
				RoomCodeSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
				SessionSettings.Settings.Add(FName(TEXT("ROOM_CODE")), RoomCodeSetting);

				bool bSuccess = SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
				if (!bSuccess)
				{
					UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete: Failed to initiate CreateSession call"));
					OnHostSessionComplete.Broadcast(false);
				}
				return;
			}
		}
		OnHostSessionComplete.Broadcast(false);
	}
}

void UGSGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete: %s, Successful: %d"), *SessionName.ToString(), bWasSuccessful);

	if (bWasSuccessful)
	{
		ActiveRoomCode = PendingRoomCode;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(777, 180.0f, FColor::Yellow, FString::Printf(TEXT("ROOM CODE: %s"), *ActiveRoomCode));
		}
		UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("/Game/Levels/SL_E")), true, TEXT("listen"));
	}

	OnHostSessionComplete.Broadcast(bWasSuccessful);
}

void UGSGameInstance::JoinGame(const FString& RoomCode)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinGame: Failed to get OnlineSubsystem"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinGame: Failed to get SessionInterface"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	PendingRoomCode = RoomCode.ToUpper().TrimStartAndEnd();

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(FName(TEXT("ROOM_CODE")), PendingRoomCode, EOnlineComparisonOp::Equals);

	bool bSuccess = SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinGame: Failed to initiate FindSessions call"));
		OnJoinSessionComplete.Broadcast(false);
	}
}

void UGSGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete, Successful: %d"), bWasSuccessful);

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Found %d sessions. Searching for Room Code: %s"), SessionSearch->SearchResults.Num(), *PendingRoomCode);

	FOnlineSessionSearchResult* TargetSession = nullptr;
	for (FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
	{
		FString FoundRoomCode;
		if (Result.Session.SessionSettings.Get(FName(TEXT("ROOM_CODE")), FoundRoomCode))
		{
			UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Checked session with Room Code: %s"), *FoundRoomCode);
			if (FoundRoomCode.Equals(PendingRoomCode, ESearchCase::IgnoreCase))
			{
				TargetSession = &Result;
				break;
			}
		}
	}

	if (TargetSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Match found. Joining session..."));
		bool bSuccess = SessionInterface->JoinSession(0, NAME_GameSession, *TargetSession);
		if (!bSuccess)
		{
			UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Failed to initiate JoinSession call"));
			OnJoinSessionComplete.Broadcast(false);
		}
	}
	else
	{


		FString TempRoomCode;
		bool bWasFiltered = SessionSearch->QuerySettings.Get(FName(TEXT("ROOM_CODE")), TempRoomCode);
		if (bWasFiltered)
		{
			UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Filtered search returned no matches. Retrying with broad unfiltered search fallback..."));
			
			SessionSearch = MakeShareable(new FOnlineSessionSearch());
			SessionSearch->MaxSearchResults = 10000;
			SessionSearch->bIsLanQuery = false;
			SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

			bool bSuccess = SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
			if (!bSuccess)
			{
				UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete Fallback: Failed to initiate FindSessions call"));
				OnJoinSessionComplete.Broadcast(false);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: No session found matching Room Code: %s"), *PendingRoomCode);
			OnJoinSessionComplete.Broadcast(false);
		}
	}
}

void UGSGameInstance::OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionCompleteInternal: %s, Result: %d"), *SessionName.ToString(), (int32)Result);

	if (Result == EOnJoinSessionCompleteResult::Success || Result == EOnJoinSessionCompleteResult::AlreadyInSession)
	{
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
		if (Subsystem)
		{
			IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
			if (SessionInterface.IsValid())
			{
				FString ConnectInfo;
				if (SessionInterface->GetResolvedConnectString(SessionName, ConnectInfo))
				{
					UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionCompleteInternal: Resolved connection: %s"), *ConnectInfo);
					APlayerController* PlayerController = GetFirstLocalPlayerController();
					if (PlayerController)
					{
						ActiveRoomCode = PendingRoomCode;
						if (GEngine)
						{
							GEngine->AddOnScreenDebugMessage(777, 180.0f, FColor::Yellow, FString::Printf(TEXT("ROOM CODE: %s"), *ActiveRoomCode));
						}
						PlayerController->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
						OnJoinSessionComplete.Broadcast(true);
						return;
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionCompleteInternal: Failed to resolve connection string"));
				}
			}
		}
	}

	OnJoinSessionComplete.Broadcast(false);
}

FString UGSGameInstance::GetActiveRoomCode() const
{
	return ActiveRoomCode;
}

void UGSGameInstance::OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult)
{
	UE_LOG(LogTemp, Warning, TEXT("OnSessionUserInviteAccepted: Successful: %d"), bWasSuccessful);

	if (bWasSuccessful)
	{
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
		if (Subsystem)
		{
			IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
			if (SessionInterface.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("OnSessionUserInviteAccepted: Joining invited session..."));
				
				FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
				if (ExistingSession != nullptr)
				{
					SessionInterface->DestroySession(NAME_GameSession);
				}

				bool bSuccess = SessionInterface->JoinSession(ControllerId, NAME_GameSession, InviteResult);
				if (!bSuccess)
				{
					UE_LOG(LogTemp, Warning, TEXT("OnSessionUserInviteAccepted: Failed to initiate JoinSession"));
					OnJoinSessionComplete.Broadcast(false);
				}
			}
		}
	}
}
