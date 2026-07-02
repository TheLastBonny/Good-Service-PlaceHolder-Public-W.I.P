// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GSGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGSHostSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGSJoinSessionComplete, bool, bWasSuccessful);

/**
 * Custom GameInstance to handle hosting and joining Steam sessions using Room Codes
 */
UCLASS()
class PROJECTF_API UGSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UGSGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	// Hosts a session with a room code. Travels to lobby map on success.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void HostGame(const FString& RoomCode, int32 MaxPlayers = 4);

	// Searches for a session with a room code. Joins it on success.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void JoinGame(const FString& RoomCode);

	// Returns the currently active Room Code.
	UFUNCTION(BlueprintPure, Category = "Multiplayer")
	FString GetActiveRoomCode() const;

	// Delegates to notify Blueprint UI of completion status
	UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
	FOnGSHostSessionComplete OnHostSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
	FOnGSJoinSessionComplete OnJoinSessionComplete;

protected:
	// Session Interface Delegate Callbacks
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

private:
	// Delegate instances
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;

	// Handles for delegates
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	// Session Search Settings
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// State variables
	FString PendingRoomCode;
	FString ActiveRoomCode;
	int32 PendingMaxPlayers;
	bool bIsHostPending;
};
