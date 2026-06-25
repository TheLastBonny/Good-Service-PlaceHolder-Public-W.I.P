#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GSPlayerState.generated.h"

UCLASS()
class PROJECTF_API AGSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AGSPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_IsReady, BlueprintReadWrite, Category = "Lobby")
	bool bIsReady;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetReady(bool bReady);

protected:
	UFUNCTION()
	void OnRep_IsReady();
};
