#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GSPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class PROJECTF_API AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleJumpTriggered();
	void HandleJumpCompleted();
};
