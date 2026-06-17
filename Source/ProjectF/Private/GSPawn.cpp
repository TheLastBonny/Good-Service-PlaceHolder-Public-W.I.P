// Fill out your copyright notice in the Description page of Project Settings.

#include "GSPawn.h"
#include "Components/CapsuleComponent.h"
#include "ChaosMover/Character/ChaosCharacterMoverComponent.h"

AGSPawn::AGSPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CapsuleComponent->InitCapsuleSize(35.f, 90.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = true;

	RootComponent = CapsuleComponent;

	MoverComponent = CreateDefaultSubobject<UChaosCharacterMoverComponent>(TEXT("MoverComponent"));

	CachedMovementInput = FVector2D::ZeroVector;
	bCachedJumpPressed = false;

	SetReplicateMovement(false);
	bReplicates = true;
}

void AGSPawn::BeginPlay()
{
	if (CapsuleComponent)
	{
		CapsuleComponent->SetMobility(EComponentMobility::Movable);
		CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsuleComponent->SetSimulatePhysics(true);
	}

	Super::BeginPlay();

	if (CapsuleComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("=== DIAGNOSTICO CAPSULE COMPONENT ==="));
		UE_LOG(LogTemp, Warning, TEXT("Profile Name: %s"), *CapsuleComponent->GetCollisionProfileName().ToString());
		UE_LOG(LogTemp, Warning, TEXT("Collision Enabled: %d"), (int32)CapsuleComponent->GetCollisionEnabled());
		
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(CapsuleComponent);
		bool bHasPhysicsObject = PrimComp && PrimComp->GetPhysicsObjectByName(NAME_None) != nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Physics Object Valid: %s"), bHasPhysicsObject ? TEXT("YES") : TEXT("NO ❌"));
		UE_LOG(LogTemp, Warning, TEXT("====================================="));
	}

	if (MoverComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("=== DIAGNOSTICO MOVER COMPONENT ==="));
		UE_LOG(LogTemp, Warning, TEXT("1. Mover es VALIDO."));
		UE_LOG(LogTemp, Warning, TEXT("2. Backend Class asignado: %s"), MoverComponent->BackendClass ? *MoverComponent->BackendClass->GetName() : TEXT("NINGUNO ❌"));
		UE_LOG(LogTemp, Warning, TEXT("3. Modo inicial (Starting Mode): %s"), *MoverComponent->StartingMovementMode.ToString());
		UE_LOG(LogTemp, Warning, TEXT("4. Cantidad de Modos registrados: %d"), MoverComponent->MovementModes.Num());
		for (auto& Elem : MoverComponent->MovementModes)
		{
			UE_LOG(LogTemp, Warning, TEXT("   - Modo: %s (%s)"), *Elem.Key.ToString(), Elem.Value ? *Elem.Value->GetName() : TEXT("NULL"));
		}
		UE_LOG(LogTemp, Warning, TEXT("5. Cantidad de Transiciones registradas: %d"), MoverComponent->Transitions.Num());
		UE_LOG(LogTemp, Warning, TEXT("=================================="));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ El Mover Component es NULO en BeginPlay!"));
	}
}

void AGSPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	static int32 TickCount = 0;
	if (++TickCount % 120 == 0 && MoverComponent)
	{
		FVector CurrentLocation = GetActorLocation();
		FVector CurrentVelocity = MoverComponent->GetVelocity();
		FName CurrentMode = MoverComponent->GetMovementModeName();
		USceneComponent* UpdatedComp = MoverComponent->GetUpdatedComponent();
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(UpdatedComp);
		bool bHasPhysicsObject = PrimComp && PrimComp->GetPhysicsObjectByName(NAME_None) != nullptr;

		// BodyInstance and Physics Handle check
		FBodyInstance* BodyInst = CapsuleComponent ? CapsuleComponent->GetBodyInstance() : nullptr;
		bool bIsValidBody = BodyInst && BodyInst->IsValidBodyInstance();
		void* ActorHandle = BodyInst ? BodyInst->GetPhysicsActorHandle() : nullptr;
		bool bSimPhysics = CapsuleComponent && CapsuleComponent->IsSimulatingPhysics();

		// Backend Components check
		UActorComponent* ChaosBackend = nullptr;
		UActorComponent* NetPhysComp = nullptr;
		TArray<UActorComponent*> Comps;
		GetComponents(Comps);
		for (UActorComponent* Comp : Comps)
		{
			if (Comp)
			{
				FString ClassName = Comp->GetClass()->GetName();
				if (ClassName == TEXT("ChaosMoverBackendComponent"))
				{
					ChaosBackend = Comp;
				}
				else if (ClassName.Contains(TEXT("NetworkPhysics")))
				{
					NetPhysComp = Comp;
				}
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("=== AGSPawn::Tick Diagnostic (Frame %d) ==="), TickCount);
		UE_LOG(LogTemp, Warning, TEXT("  Location: %s"), *CurrentLocation.ToString());
		UE_LOG(LogTemp, Warning, TEXT("  Velocity: %s"), *CurrentVelocity.ToString());
		UE_LOG(LogTemp, Warning, TEXT("  Active Movement Mode: %s"), *CurrentMode.ToString());
		UE_LOG(LogTemp, Warning, TEXT("  Updated Component: %s"), UpdatedComp ? *UpdatedComp->GetName() : TEXT("NULL"));
		UE_LOG(LogTemp, Warning, TEXT("  Physics Object Valid: %s"), bHasPhysicsObject ? TEXT("YES") : TEXT("NO ❌"));
		UE_LOG(LogTemp, Warning, TEXT("  BodyInstance Valid: %s"), bIsValidBody ? TEXT("YES") : TEXT("NO ❌"));
		UE_LOG(LogTemp, Warning, TEXT("  Physics Actor Handle: %s"), ActorHandle ? TEXT("VALID") : TEXT("NULL ❌"));
		UE_LOG(LogTemp, Warning, TEXT("  Simulating Physics: %s"), bSimPhysics ? TEXT("YES") : TEXT("NO"));
		UE_LOG(LogTemp, Warning, TEXT("  ChaosMoverBackend Found: %s"), ChaosBackend ? TEXT("YES") : TEXT("NO ❌"));
		UE_LOG(LogTemp, Warning, TEXT("  NetworkPhysicsComponent Found: %s"), NetPhysComp ? TEXT("YES") : TEXT("NO ❌"));
		UE_LOG(LogTemp, Warning, TEXT("=========================================="));
	}
}

void AGSPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AGSPawn::RequestMove_Implementation(const FVector2D& MovementVector)
{
	UE_LOG(LogTemp, Warning, TEXT("AGSPawn::RequestMove_Implementation called. Vector: X=%f, Y=%f"), MovementVector.X, MovementVector.Y);
	CachedMovementInput = MovementVector;
}

void AGSPawn::RequestJump_Implementation(bool bIsJumping)
{
	UE_LOG(LogTemp, Warning, TEXT("AGSPawn::RequestJump_Implementation called. bIsJumping: %d"), bIsJumping ? 1 : 0);
	bCachedJumpPressed = bIsJumping;
}

void AGSPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	static int32 CallCount = 0;
	if (++CallCount % 30 == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGSPawn::ProduceInput_Implementation called (Frame %d). Input Intent: X=%f, Y=%f. Jump Pressed: %d"), 
			CallCount, CachedMovementInput.X, CachedMovementInput.Y, bCachedJumpPressed ? 1 : 0);
	}

	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	FVector MoveDirection = FVector::ZeroVector;
	if (!CachedMovementInput.IsZero())
	{
		const FRotator ControlRotation = GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		MoveDirection = (ForwardVector * CachedMovementInput.Y) + (RightVector * CachedMovementInput.X);
		MoveDirection.Normalize();
	}

	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
	CharacterInputs.bIsJumpPressed = bCachedJumpPressed;
	CharacterInputs.OrientationIntent = MoveDirection;
}
