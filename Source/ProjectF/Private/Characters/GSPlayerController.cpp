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
#include "Core/GSGameInstance.h"
#include "Characters/GSPawn.h"
#include "Core/GSGameplayTags.h"
#include "Engine/OverlapResult.h"
#include "Components/GSGrabbableComponent.h"
#include "Core/GSCameraTriggerVolume.h"

AGSPlayerController::AGSPlayerController()
{
	ThrowArcHeight = 300.0f;
	LastGrabbedActor = nullptr;
	bIsSpecialModifierDown = false;
	bLastReleaseWasSpecial = false;
	bInitialCameraVolumeChecked = false;
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


		if (UGSGameInstance* GSGI = Cast<UGSGameInstance>(GetGameInstance()))
		{
			FString Code = GSGI->GetActiveRoomCode();
			if (!Code.IsEmpty())
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(777, 300.0f, FColor::Yellow, FString::Printf(TEXT("ROOM CODE: %s"), *Code));
				}
				
			}
		}
	}
}

void AGSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);


	if (IsLocalController())
	{
		if (!bInitialCameraVolumeChecked)
		{
			APawn* P = GetPawn();
			if (P)
			{
				bInitialCameraVolumeChecked = true;
				P->UpdateOverlaps();
				TArray<AActor*> OverlappingActors;
				P->GetOverlappingActors(OverlappingActors);
				for (AActor* Actor : OverlappingActors)
				{
					if (AGSCameraTriggerVolume* Volume = Cast<AGSCameraTriggerVolume>(Actor))
					{
						AActor* Target = Volume->GetCustomCameraTarget() ? Volume->GetCustomCameraTarget() : Volume;
						PushCameraVolume(Volume, Target, Volume->GetBlendTime(), Volume->GetBlendFunction(), Volume->GetBlendExp());
					}
				}
			}
		}

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

		if (SpecialModifierAction)
		{
			EnhancedInputComponent->BindAction(SpecialModifierAction, ETriggerEvent::Started, this, &AGSPlayerController::HandleSpecialPressed);
			EnhancedInputComponent->BindAction(SpecialModifierAction, ETriggerEvent::Completed, this, &AGSPlayerController::HandleSpecialReleased);
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
	

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			if (AGSPawn* GSPawn = Cast<AGSPawn>(ControlledPawn))
			{
				if (GSPawn->GetAbilityTagForSlot(InputTag) == GSGameplayTags::Ability_Grab)
				{
					TArray<FOverlapResult> Overlaps;
					FCollisionQueryParams Params;
					Params.AddIgnoredActor(ControlledPawn);
					float GrabRadius = 120.0f;
					float GrabForwardOffset = 80.0f;
					FCollisionShape Shape = FCollisionShape::MakeSphere(GrabRadius);
					FVector ScanLocation = ControlledPawn->GetActorLocation() + (ControlledPawn->GetActorForwardVector() * GrabForwardOffset);

					FCollisionObjectQueryParams ObjectQueryParams;
					ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
					ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

					GetWorld()->OverlapMultiByObjectType(Overlaps, ScanLocation, FQuat::Identity, ObjectQueryParams, Shape, Params);

					float MinDist = TNumericLimits<float>::Max();
					AActor* BestTarget = nullptr;
					for (const FOverlapResult& Overlap : Overlaps)
					{
						AActor* Candidate = Overlap.GetActor();
						if (!Candidate) continue;

						UGSGrabbableComponent* GrabComp = Candidate->FindComponentByClass<UGSGrabbableComponent>();
						if (GrabComp && !GrabComp->IsGrabbed())
						{
							float Dist = FVector::DistSquared(ControlledPawn->GetActorLocation(), Candidate->GetActorLocation());
							if (Dist < MinDist)
							{
								MinDist = Dist;
								BestTarget = Candidate;
							}
						}
					}

					if (BestTarget)
					{
						LastGrabbedActor = BestTarget;
						Server_SetGrabbedActor(BestTarget);
					}
					else
					{
						LastGrabbedActor = nullptr;
						Server_SetGrabbedActor(nullptr);
					}
				}
			}
			
			IGSPlayerInterface::Execute_RequestAbilityByTag(ControlledPawn, InputTag);
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
		
	}
	else
	{
		
	}
	LastAimTargetLocation = ClientAimTarget;
	bLastReleaseWasSpecial = bIsSpecialModifierDown;

	

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			
			IGSPlayerInterface::Execute_RequestAbilityReleasedByTag(ControlledPawn, InputTag);
		}
		else
		{
			
		}
	}

	if (!HasAuthority())
	{
		
		Server_Input_AbilityReleased(InputTag, ClientAimTarget, bIsSpecialModifierDown);
	}
}

void AGSPlayerController::Server_Input_AbilityReleased_Implementation(FGameplayTag InputTag, FVector ClientAimTarget, bool bIsSpecialDown)
{
	
	LastAimTargetLocation = ClientAimTarget;
	bLastReleaseWasSpecial = bIsSpecialDown;
	
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ControlledPawn->Implements<UGSPlayerInterface>())
		{
			
			IGSPlayerInterface::Execute_RequestAbilityReleasedByTag(ControlledPawn, InputTag);
		}
	}
}

void AGSPlayerController::HandleSpecialPressed()
{
	bIsSpecialModifierDown = true;
}

void AGSPlayerController::HandleSpecialReleased()
{
	bIsSpecialModifierDown = false;
}

void AGSPlayerController::Server_SetGrabbedActor_Implementation(AActor* InActor)
{
	
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

	
	bShowMouseCursor = true;

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		FVector2D ScreenPosition;
		if (ProjectWorldLocationToScreen(ControlledPawn->GetActorLocation(), ScreenPosition))
		{
			
			SetMouseLocation(FMath::RoundToInt(ScreenPosition.X), FMath::RoundToInt(ScreenPosition.Y));
		}
	}
}

void AGSPlayerController::HideAimCursor()
{
	if (!IsLocalController()) { return; }

	
	bShowMouseCursor = false;
}

void AGSPlayerController::PushCameraVolume(AActor* Volume, AActor* CameraTarget, float BlendTime, EViewTargetBlendFunction BlendFunction, float BlendExp)
{
	if (!Volume || !CameraTarget)
	{
		return;
	}

	int32 Index = CameraVolumeStack.IndexOfByPredicate([Volume](const FCameraVolumeState& State) {
		return State.Volume == Volume;
	});

	if (Index == INDEX_NONE)
	{
		FCameraVolumeState NewState;
		NewState.Volume = Volume;
		NewState.CameraTarget = CameraTarget;
		NewState.BlendTime = BlendTime;
		NewState.BlendFunction = BlendFunction;
		NewState.BlendExp = BlendExp;
		CameraVolumeStack.Add(NewState);

		SetViewTargetWithBlend(CameraTarget, BlendTime, BlendFunction, BlendExp);
	}
}

void AGSPlayerController::PopCameraVolume(AActor* Volume, float BlendTime, EViewTargetBlendFunction BlendFunction, float BlendExp)
{
	if (!Volume)
	{
		return;
	}

	int32 Index = CameraVolumeStack.IndexOfByPredicate([Volume](const FCameraVolumeState& State) {
		return State.Volume == Volume;
	});

	if (Index != INDEX_NONE)
	{
		CameraVolumeStack.RemoveAt(Index);

		if (CameraVolumeStack.Num() > 0)
		{
			const FCameraVolumeState& TopState = CameraVolumeStack.Last();
			if (AActor* Target = TopState.CameraTarget.Get())
			{
				SetViewTargetWithBlend(Target, TopState.BlendTime, TopState.BlendFunction, TopState.BlendExp);
			}
		}
		else
		{
			if (APawn* ControlledPawn = GetPawn())
			{
				SetViewTargetWithBlend(ControlledPawn, BlendTime, BlendFunction, BlendExp);
			}
		}
	}
}

void AGSPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	bInitialCameraVolumeChecked = false;
}
