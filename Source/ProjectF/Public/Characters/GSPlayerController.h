#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Camera/PlayerCameraManager.h"
#include "GSPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UGSInputConfig;
struct FInputActionValue;

USTRUCT(BlueprintType)
struct FCameraVolumeState
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Volume;

	UPROPERTY()
	TWeakObjectPtr<AActor> CameraTarget;

	UPROPERTY()
	float BlendTime = 0.5f;

	UPROPERTY()
	TEnumAsByte<EViewTargetBlendFunction> BlendFunction = VTBlend_Cubic;

	UPROPERTY()
	float BlendExp = 2.0f;
};

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<const UInputAction> SpecialModifierAction;

	UPROPERTY(BlueprintReadWrite, Category = "Input")
	bool bIsSpecialModifierDown;

	UPROPERTY(BlueprintReadWrite, Category = "Input")
	bool bLastReleaseWasSpecial;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void ShowAimCursor();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void HideAimCursor();

	UFUNCTION(Server, Reliable)
	void Server_Input_AbilityReleased(FGameplayTag InputTag, FVector ClientAimTarget, bool bIsSpecialDown);

	UFUNCTION(Server, Reliable)
	void Server_SetGrabbedActor(AActor* InActor);

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	virtual void AcknowledgePossession(APawn* P) override;

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
	void HandleSpecialPressed();
	void HandleSpecialReleased();

	void Input_AbilityActivate(FGameplayTag InputTag);
	void Input_AbilityReleased(FGameplayTag InputTag);

public:
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void PushCameraVolume(AActor* Volume, AActor* CameraTarget, float BlendTime, EViewTargetBlendFunction BlendFunction, float BlendExp);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void PopCameraVolume(AActor* Volume, float BlendTime, EViewTargetBlendFunction BlendFunction, float BlendExp);

private:
	UPROPERTY(Transient)
	TArray<FCameraVolumeState> CameraVolumeStack;

	bool bInitialCameraVolumeChecked;

};
