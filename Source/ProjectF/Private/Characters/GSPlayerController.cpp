#include "Characters/GSPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Characters/GSPlayerInterface.h"
#include "Input/GSInputConfig.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/GSGameInstance.h"

AGSPlayerController::AGSPlayerController()
{
	ThrowArcHeight = 300.0f;
	LastGrabbedActor = nullptr;
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

		// Print Room Code to Screen if available in GameInstance (persists after level travel)
		if (UGSGameInstance* GSGI = Cast<UGSGameInstance>(GetGameInstance()))
		{
			FString Code = GSGI->GetActiveRoomCode();
			if (!Code.IsEmpty())
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(777, 300.0f, FColor::Yellow, FString::Printf(TEXT("ROOM CODE: %s"), *Code));
				}
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG] Local Player Controller: ROOM CODE is %s"), *Code);
			}
		}
	}
}

void AGSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// If local controller, draw aiming line when aiming tag is active
	if (IsLocalController())
	{
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn && ControlledPawn->Implements<UGSPlayerInterface>())
		{
			IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(ControlledPawn);
			UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
			if (ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Aiming"))))
			{
				FHitResult HitResult;
				if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
				{
					FVector TargetLoc = HitResult.Location;
					FVector StartLoc = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
					
					// Draw debug line and sphere for 1 frame
					DrawDebugLine(GetWorld(), StartLoc, TargetLoc, FColor::Orange, false, -1.0f, 0, 2.0f);
					DrawDebugSphere(GetWorld(), TargetLoc, 20.0f, 16, FColor::Orange, false, -1.0f, 0, 1.5f);
				}
			}
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

		if (AdjustArcAction)
		{
			EnhancedInputComponent->BindAction(AdjustArcAction, ETriggerEvent::Triggered, this, &AGSPlayerController::HandleAdjustArc);
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
	if (!IsLocalController()) { return; }

	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController::Input_AbilityActivate (Local Only): InputTag: %s, Pawn: %s, LastGrabbedActor: %s"), 
		*NetRole, *InputTag.ToString(), GetPawn() ? *GetPawn()->GetName() : TEXT("NULL"), 
		LastGrabbedActor ? *LastGrabbedActor->GetName() : TEXT("NULL"));

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController: Executing local RequestAbilityByTag on Pawn %s."), *NetRole, *ControlledPawn->GetName());
			IGSPlayerInterface::Execute_RequestAbilityByTag(ControlledPawn, InputTag);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController: ControlledPawn %s does NOT implement UGSPlayerInterface!"), *NetRole, *ControlledPawn->GetName());
		}
	}
}

void AGSPlayerController::Input_AbilityReleased(FGameplayTag InputTag)
{
	if (!IsLocalController()) { return; }

	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	FVector ClientAimTarget = FVector::ZeroVector;
	
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		ClientAimTarget = HitResult.Location;
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController: Client trace hit location: %s, Component: %s, Actor: %s"), 
			*NetRole, *ClientAimTarget.ToString(), 
			HitResult.GetComponent() ? *HitResult.GetComponent()->GetName() : TEXT("NULL"),
			HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("NULL"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController: Client trace under cursor missed."), *NetRole);
	}
	LastAimTargetLocation = ClientAimTarget;

	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController::Input_AbilityReleased (Local Only): InputTag: %s, Pawn: %s, LastGrabbedActor: %s, LastAimTargetLocation: %s"), 
		*NetRole, *InputTag.ToString(), GetPawn() ? *GetPawn()->GetName() : TEXT("NULL"), 
		LastGrabbedActor ? *LastGrabbedActor->GetName() : TEXT("NULL"), *LastAimTargetLocation.ToString());

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController: Executing local RequestAbilityReleasedByTag on Pawn %s."), *NetRole, *ControlledPawn->GetName());
			IGSPlayerInterface::Execute_RequestAbilityReleasedByTag(ControlledPawn, InputTag);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController: ControlledPawn %s does NOT implement UGSPlayerInterface!"), *NetRole, *ControlledPawn->GetName());
		}
	}

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPlayerController: Client sending Server_Input_AbilityReleased RPC with Target: %s"), *NetRole, *ClientAimTarget.ToString());
		Server_Input_AbilityReleased(InputTag, ClientAimTarget);
	}
}

void AGSPlayerController::Server_Input_AbilityReleased_Implementation(FGameplayTag InputTag, FVector ClientAimTarget)
{
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] AGSPlayerController::Server_Input_AbilityReleased RPC received on Server. Target: %s"), *ClientAimTarget.ToString());
	LastAimTargetLocation = ClientAimTarget;
	
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] AGSPlayerController: Executing Server-side RequestAbilityReleasedByTag on Pawn %s."), *ControlledPawn->GetName());
			IGSPlayerInterface::Execute_RequestAbilityReleasedByTag(ControlledPawn, InputTag);
		}
	}
}

void AGSPlayerController::Server_SetGrabbedActor_Implementation(AActor* InActor)
{
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] AGSPlayerController::Server_SetGrabbedActor RPC received on Server. Actor: %s"), InActor ? *InActor->GetName() : TEXT("NULL"));
	LastGrabbedActor = InActor;
}

void AGSPlayerController::HandleAdjustArc(const FInputActionValue& Value)
{
	float AdjustValue = Value.Get<float>();
	ThrowArcHeight = FMath::Clamp(ThrowArcHeight + (AdjustValue * 50.0f), 0.0f, 1000.0f);
}

void AGSPlayerController::ShowAimCursor()
{
	if (!IsLocalController()) { return; }

	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] AGSPlayerController::ShowAimCursor called. Setting mouse cursor to visible."));
	bShowMouseCursor = true;

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		FVector2D ScreenPosition;
		if (ProjectWorldLocationToScreen(ControlledPawn->GetActorLocation(), ScreenPosition))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] AGSPlayerController::ShowAimCursor: Centering mouse on screen position: %s"), *ScreenPosition.ToString());
			SetMouseLocation(FMath::RoundToInt(ScreenPosition.X), FMath::RoundToInt(ScreenPosition.Y));
		}
	}
}

void AGSPlayerController::HideAimCursor()
{
	if (!IsLocalController()) { return; }

	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] AGSPlayerController::HideAimCursor called. Hiding mouse cursor."));
	bShowMouseCursor = false;
}
