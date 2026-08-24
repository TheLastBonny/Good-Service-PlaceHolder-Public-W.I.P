

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


	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void HostGame(const FString& RoomCode, int32 MaxPlayers = 4);


	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void JoinGame(const FString& RoomCode);


	UFUNCTION(BlueprintPure, Category = "Multiplayer")
	FString GetActiveRoomCode() const;


	UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
	FOnGSHostSessionComplete OnHostSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
	FOnGSJoinSessionComplete OnJoinSessionComplete;

protected:

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult);

private:

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;


	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle OnSessionUserInviteAcceptedDelegateHandle;


	TSharedPtr<FOnlineSessionSearch> SessionSearch;


	FString PendingRoomCode;
	FString ActiveRoomCode;
	int32 PendingMaxPlayers;
	bool bIsHostPending;
};
