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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UGSInputConfig> InputConfig;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleJumpTriggered();
	void HandleJumpCompleted();

	void Input_AbilityActivate(FGameplayTag InputTag);
	void Input_AbilityReleased(FGameplayTag InputTag);
};
