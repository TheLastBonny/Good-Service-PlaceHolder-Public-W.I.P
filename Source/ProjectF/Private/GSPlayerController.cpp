#include "GSPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GSPlayerInterface.h"

AGSPlayerController::AGSPlayerController()
{
}

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("=== AGSPlayerController::BeginPlay ==="));

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
			UE_LOG(LogTemp, Warning, TEXT("Successfully added InputMappingContext: %s"), *InputMappingContext->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InputMappingContext is NULL in GSPlayerController!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get UEnhancedInputLocalPlayerSubsystem!"));
	}
}

void AGSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UE_LOG(LogTemp, Warning, TEXT("=== AGSPlayerController::SetupInputComponent ==="));

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGSPlayerController::HandleMove);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AGSPlayerController::HandleMove);
			UE_LOG(LogTemp, Warning, TEXT("Bound MoveAction: %s"), *MoveAction->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MoveAction is NULL!"));
		}

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGSPlayerController::HandleJumpTriggered);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGSPlayerController::HandleJumpCompleted);
			UE_LOG(LogTemp, Warning, TEXT("Bound JumpAction: %s"), *JumpAction->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("JumpAction is NULL!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InputComponent is not an EnhancedInputComponent!"));
	}
}

void AGSPlayerController::HandleMove(const FInputActionValue& Value)
{
	FVector2D MoveVector = Value.Get<FVector2D>();
	APawn* ControlledPawn = GetPawn();
	
	UE_LOG(LogTemp, Warning, TEXT("AGSPlayerController::HandleMove called. Vector: X=%f, Y=%f. Pawn: %s"), 
		MoveVector.X, MoveVector.Y, ControlledPawn ? *ControlledPawn->GetName() : TEXT("NULL"));

	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			IGSPlayerInterface::Execute_RequestMove(ControlledPawn, MoveVector);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Controlled Pawn (%s) does NOT implement IGSPlayerInterface!"), *ControlledPawn->GetName());
		}
	}
}

void AGSPlayerController::HandleJumpTriggered()
{
	APawn* ControlledPawn = GetPawn();
	UE_LOG(LogTemp, Warning, TEXT("AGSPlayerController::HandleJumpTriggered called. Pawn: %s"), 
		ControlledPawn ? *ControlledPawn->GetName() : TEXT("NULL"));

	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			IGSPlayerInterface::Execute_RequestJump(ControlledPawn, true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Controlled Pawn (%s) does NOT implement IGSPlayerInterface!"), *ControlledPawn->GetName());
		}
	}
}

void AGSPlayerController::HandleJumpCompleted()
{
	APawn* ControlledPawn = GetPawn();
	UE_LOG(LogTemp, Warning, TEXT("AGSPlayerController::HandleJumpCompleted called. Pawn: %s"), 
		ControlledPawn ? *ControlledPawn->GetName() : TEXT("NULL"));

	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			IGSPlayerInterface::Execute_RequestJump(ControlledPawn, false);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Controlled Pawn (%s) does NOT implement IGSPlayerInterface!"), *ControlledPawn->GetName());
		}
	}
}
