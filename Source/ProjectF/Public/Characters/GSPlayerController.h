#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "GSPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UGSInputConfig;
struct FInputActionValue;

UCLASS()
class PROJECTF_API AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch")
	float ThrowArcHeight;

	UPROPERTY(BlueprintReadWrite, Category = "Launch")
	FVector LastAimTargetLocation;

	UPROPERTY(BlueprintReadWrite, Category = "Grab")
	TObjectPtr<AActor> LastGrabbedActor;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void ShowAimCursor();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void HideAimCursor();

	UFUNCTION(Server, Reliable)
	void Server_Input_AbilityReleased(FGameplayTag InputTag, FVector ClientAimTarget);

	UFUNCTION(Server, Reliable)
	void Server_SetGrabbedActor(AActor* InActor);

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UGSInputConfig> InputConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AdjustArcAction;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleJumpTriggered();
	void HandleJumpCompleted();
	void HandleAdjustArc(const FInputActionValue& Value);

	void Input_AbilityActivate(FGameplayTag InputTag);
	void Input_AbilityReleased(FGameplayTag InputTag);

};
