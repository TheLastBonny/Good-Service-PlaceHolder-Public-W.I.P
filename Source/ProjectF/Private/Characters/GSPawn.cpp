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
#include "Core/GSGameplayTags.h"
#include "Characters/GSPlayerController.h"
#include "Components/GSGrabbableComponent.h"
#include "Engine/OverlapResult.h"

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
	TickLogTimer = 0.0f;

	SetReplicateMovement(false);
	bReplicates = true;
}

UAbilitySystemComponent* AGSPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSPawn::BeginPlay()
{
	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn::BeginPlay: Pawn %s, Location: %s"), 
		*NetRole, *GetName(), *GetActorLocation().ToString());

	if (CapsuleComponent)
	{
		CapsuleComponent->SetMobility(EComponentMobility::Movable);
		CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsuleComponent->SetSimulatePhysics(false);
		CapsuleComponent->SetUpdateKinematicFromSimulation(false);
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn::BeginPlay: Configured CapsuleComponent: Profile = Pawn, Enabled = QueryAndPhysics"), *NetRole);
	}

	Super::BeginPlay();
}

void AGSPawn::PossessedBy(AController* NewController)
{
	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn::PossessedBy: Pawn %s possessed by controller %s"), 
		*NetRole, *GetName(), NewController ? *NewController->GetName() : TEXT("NULL"));
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
	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn::RequestAbilityByTag called. Pawn: %s, InputTag: %s"), 
		*NetRole, *GetName(), *InputTag.ToString());

	if (AbilitySystemComponent)
	{
		const FGameplayTag AbilityTag = GetAbilityTagForSlot(InputTag);
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: InputTag [%s] maps to AbilityTag [%s]"), 
			*NetRole, *InputTag.ToString(), *AbilityTag.ToString());

		if (!AbilityTag.IsValid()) 
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: AbilityTag is INVALID!"), *NetRole);
			return; 
		}

		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: Listing all activatable abilities on ASC:"), *NetRole);
		int32 Index = 0;
		for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (Spec.Ability)
			{
				FString SpecTags = TEXT("");
				for (const FGameplayTag& Tag : Spec.Ability->GetAssetTags())
				{
					SpecTags += Tag.ToString() + TEXT(", ");
				}
				for (const FGameplayTag& Tag : Spec.Ability->AbilityTags)
				{
					SpecTags += Tag.ToString() + TEXT(", ");
				}
				UE_LOG(LogTemp, Warning, TEXT("  [%d] Ability: %s, Handle: %s, IsActive: %d, Tags: [%s]"), 
					Index++, *Spec.Ability->GetName(), *Spec.Handle.ToString(), Spec.IsActive(), *SpecTags);
			}
		}

		FScopedAbilityListLock AbilityScopeLock(*AbilitySystemComponent);

		int32 MatchCount = 0;
		for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			if (Spec.Ability && (Spec.Ability->GetAssetTags().HasTag(AbilityTag) || Spec.Ability->AbilityTags.HasTag(AbilityTag)))
			{
				MatchCount++;
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: Found matching ability spec: %s (Handle: %s). IsActive: %d. Calling AbilitySpecInputPressed."),
					*NetRole, *Spec.Ability->GetName(), *Spec.Handle.ToString(), Spec.IsActive());
				AbilitySystemComponent->AbilitySpecInputPressed(Spec);
				if (!Spec.IsActive())
				{
					UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: Activating ability spec %s"), *NetRole, *Spec.Handle.ToString());
					AbilitySystemComponent->TryActivateAbility(Spec.Handle);
				}
			}
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
		if (MatchCount == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: No matching activatable ability found on ASC for tag %s"), *NetRole, *AbilityTag.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: RequestAbilityByTag: ASC is NULL!"), *NetRole);
	}
}

void AGSPawn::RequestAbilityReleasedByTag_Implementation(const FGameplayTag& InputTag)
{
	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn::RequestAbilityReleasedByTag called. Pawn: %s, InputTag: %s"), 
		*NetRole, *GetName(), *InputTag.ToString());

	if (AbilitySystemComponent)
	{
		const FGameplayTag AbilityTag = GetAbilityTagForSlot(InputTag);
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: Released Slot Tag [%s] mapped to Ability Tag [%s]"), 
			*NetRole, *InputTag.ToString(), *AbilityTag.ToString());

		if (!AbilityTag.IsValid()) 
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: AbilityTag for Slot Tag [%s] is INVALID!"), *NetRole, *InputTag.ToString());
			return; 
		}

		FScopedAbilityListLock AbilityScopeLock(*AbilitySystemComponent);

		int32 MatchCount = 0;
		for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			if (Spec.Ability && (Spec.Ability->GetAssetTags().HasTag(AbilityTag) || Spec.Ability->AbilityTags.HasTag(AbilityTag)))
			{
				MatchCount++;
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: Found matching ability spec for release: %s. IsActive = %d. Calling AbilitySpecInputReleased."),
					*NetRole, *Spec.Ability->GetName(), Spec.IsActive());
				AbilitySystemComponent->AbilitySpecInputReleased(Spec);
			}
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
		if (MatchCount == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: No matching activatable ability found on ASC for release tag %s"), *NetRole, *AbilityTag.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] AGSPawn: RequestAbilityReleasedByTag: ASC is NULL!"), *NetRole);
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

	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GSGameplayTags::State_Aiming))
	{
		FVector AimDirection = GetActorForwardVector();
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult HitResult;
			if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
			{
				FVector Dir = HitResult.Location - GetActorLocation();
				Dir.Z = 0.0f;
				if (Dir.Normalize())
				{
					AimDirection = Dir;
				}
			}
		}
		CharacterInputs.OrientationIntent = AimDirection;
	}
	else
	{
		CharacterInputs.OrientationIntent = MoveDirection;
	}

	CharacterInputs.ControlRotation = GetControlRotation();

	bCachedJumpJustPressed = false;
}
