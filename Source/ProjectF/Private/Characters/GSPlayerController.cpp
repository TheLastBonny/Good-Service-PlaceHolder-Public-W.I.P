#include "Characters/GSPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Characters/GSPlayerInterface.h"
#include "Input/GSInputConfig.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"

AGSPlayerController::AGSPlayerController()
{
}

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
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
}

void AGSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGSPlayerController::HandleMove);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AGSPlayerController::HandleMove);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MoveAction is NULL!"));
		}

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGSPlayerController::HandleJumpTriggered);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGSPlayerController::HandleJumpCompleted);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("JumpAction is NULL!"));
		}

		if (InputConfig)
		{
			for (const FGSInputAction& Action : InputConfig->AbilityInputActions)
			{
				if (Action.InputAction && Action.InputTag.IsValid())
				{
					EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Started, this, &AGSPlayerController::Input_AbilityActivate, Action.InputTag);
					EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &AGSPlayerController::Input_AbilityReleased, Action.InputTag);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("InputConfig is NULL in GSPlayerController!"));
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

void AGSPlayerController::Input_AbilityActivate(FGameplayTag InputTag)
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			IGSPlayerInterface::Execute_RequestAbilityByTag(ControlledPawn, InputTag);
		}
	}
}

void AGSPlayerController::Input_AbilityReleased(FGameplayTag InputTag)
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			IGSPlayerInterface::Execute_RequestAbilityReleasedByTag(ControlledPawn, InputTag);
		}
	}
}
