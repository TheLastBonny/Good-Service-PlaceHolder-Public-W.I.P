#include "Characters/GSPawn.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DefaultMovementSet/NavMoverComponent.h"
#include "AbilitySystemComponent.h"
#include "Attributes/GSHealthAttributeSet.h"
#include "Attributes/GSMovementAttributeSet.h"
#include "Attributes/GSPatienceAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "GameFramework/PlayerState.h"

AGSPawn::AGSPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CapsuleComponent->InitCapsuleSize(35.f, 90.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(true);
	CapsuleComponent->bDynamicObstacle = true;
	RootComponent = CapsuleComponent;

	MoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("MoverComponent"));
	NavMoverComponent = CreateDefaultSubobject<UNavMoverComponent>(TEXT("NavMoverComponent"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	HealthSet = CreateDefaultSubobject<UGSHealthAttributeSet>(TEXT("HealthSet"));
	MovementSet = CreateDefaultSubobject<UGSMovementAttributeSet>(TEXT("MovementSet"));
	PatienceSet = CreateDefaultSubobject<UGSPatienceAttributeSet>(TEXT("PatienceSet"));

	CachedMovementInput = FVector2D::ZeroVector;
	bCachedJumpPressed = false;
	bCachedJumpJustPressed = false;

	SetReplicateMovement(false);
	bReplicates = true;
}

UAbilitySystemComponent* AGSPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSPawn::BeginPlay()
{
	if (CapsuleComponent)
	{
		CapsuleComponent->SetMobility(EComponentMobility::Movable);
		CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsuleComponent->SetSimulatePhysics(false);
		CapsuleComponent->SetUpdateKinematicFromSimulation(false);
	}

	Super::BeginPlay();
}

void AGSPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}

void AGSPawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
}

void AGSPawn::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitAbilityActorInfo();
}

void AGSPawn::InitAbilityActorInfo()
{
	if (!AbilitySystemComponent) { return; }

	if (APlayerState* PS = GetPlayerState())
	{
		AbilitySystemComponent->InitAbilityActorInfo(PS, this);
	}
	else
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (MovementSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			MovementSet->GetWalkSpeedAttribute()).RemoveAll(this);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			MovementSet->GetWalkSpeedAttribute()).AddUObject(this, &AGSPawn::OnWalkSpeedChanged);

		if (MoverComponent)
		{
			if (UCommonLegacyMovementSettings* MoveSettings = MoverComponent->FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
			{
				MoveSettings->MaxSpeed = MovementSet->GetWalkSpeed();
			}
		}
	}
}

void AGSPawn::OnWalkSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (MoverComponent)
	{
		if (UCommonLegacyMovementSettings* MoveSettings = MoverComponent->FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
		{
			MoveSettings->MaxSpeed = Data.NewValue;
		}
	}
}

void AGSPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGSPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AGSPawn::RequestMove_Implementation(const FVector2D& MovementVector)
{
	CachedMovementInput = MovementVector;
}

void AGSPawn::RequestJump_Implementation(bool bIsJumping)
{
	if (bIsJumping)
	{
		bCachedJumpJustPressed = !bCachedJumpPressed;
	}
	else
	{
		bCachedJumpJustPressed = false;
	}
	bCachedJumpPressed = bIsJumping;
}

void AGSPawn::RequestAbilityByTag_Implementation(const FGameplayTag& InputTag)
{
	if (AbilitySystemComponent)
	{
		const FGameplayTag AbilityTag = GetAbilityTagForSlot(InputTag);
		if (!AbilityTag.IsValid()) { return; }

		FGameplayTagContainer TagContainer(AbilityTag);
		AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
	}
}

void AGSPawn::RequestAbilityReleasedByTag_Implementation(const FGameplayTag& InputTag)
{
	if (AbilitySystemComponent)
	{
		const FGameplayTag AbilityTag = GetAbilityTagForSlot(InputTag);
		if (!AbilityTag.IsValid()) { return; }

		FScopedAbilityListLock AbilityScopeLock(*AbilitySystemComponent);

		for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(AbilityTag))
			{
				AbilitySystemComponent->AbilitySpecInputReleased(Spec);
			}
		}
	}
}

void AGSPawn::AssignAbilityToSlot(FGameplayTag SlotTag, FGameplayTag AbilityTag)
{
	AbilitySlotMap.Add(SlotTag, AbilityTag);
}

FGameplayTag AGSPawn::GetAbilityTagForSlot(FGameplayTag SlotTag) const
{
	const FGameplayTag* Found = AbilitySlotMap.Find(SlotTag);
	return Found ? *Found : FGameplayTag::EmptyTag;
}

void AGSPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	FVector MoveDirection = FVector::ZeroVector;

	if (NavMoverComponent)
	{
		FVector NavMoveInputIntent = FVector::ZeroVector;
		FVector NavMoveInputVelocity = FVector::ZeroVector;
		if (NavMoverComponent->ConsumeNavMovementData(NavMoveInputIntent, NavMoveInputVelocity))
		{
			MoveDirection = NavMoveInputIntent;
		}
	}

	if (MoveDirection.IsNearlyZero() && !CachedMovementInput.IsZero())
	{
		FRotator BaseRotation = FRotator::ZeroRotator;

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
			{
				BaseRotation = CameraManager->GetCameraRotation();
			}
			else
			{
				BaseRotation = PC->GetControlRotation();
			}
		}
		else
		{
			BaseRotation = GetControlRotation();
		}

		const FRotator YawRotation(0.f, BaseRotation.Yaw, 0.f);

		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		MoveDirection = (ForwardVector * CachedMovementInput.Y) + (RightVector * CachedMovementInput.X);
		MoveDirection.Normalize();
	}

	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);

	CharacterInputs.bIsJumpPressed = bCachedJumpPressed;
	CharacterInputs.bIsJumpJustPressed = bCachedJumpJustPressed;
	CharacterInputs.OrientationIntent = MoveDirection;
	CharacterInputs.ControlRotation = GetControlRotation();

	bCachedJumpJustPressed = false;
}
