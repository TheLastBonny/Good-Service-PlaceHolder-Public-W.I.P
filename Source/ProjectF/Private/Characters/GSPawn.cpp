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
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "DrawDebugHelpers.h"
#include "DataAssets/GSEmoteDefinition.h"
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
	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	
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
	

	if (AbilitySystemComponent)
	{
		const FGameplayTag AbilityTag = GetAbilityTagForSlot(InputTag);
		

		if (!AbilityTag.IsValid()) 
		{
			
			return; 
		}

		
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
				
				AbilitySystemComponent->AbilitySpecInputPressed(Spec);
				if (!Spec.IsActive())
				{
					
					AbilitySystemComponent->TryActivateAbility(Spec.Handle);
				}
			}
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
		if (MatchCount == 0)
		{
			
		}
	}
	else
	{
		
	}
}

void AGSPawn::RequestAbilityReleasedByTag_Implementation(const FGameplayTag& InputTag)
{
	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	

	if (AbilitySystemComponent)
	{
		const FGameplayTag AbilityTag = GetAbilityTagForSlot(InputTag);
		

		if (!AbilityTag.IsValid()) 
		{
			
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
				
				AbilitySystemComponent->AbilitySpecInputReleased(Spec);
			}
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
		if (MatchCount == 0)
		{
			
		}
	}
	else
	{
		
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

void AGSPawn::MulticastPlayEmoteSound_Implementation(UGSEmoteDefinition* EmoteDef)
{
	if (!EmoteDef) return;

	MulticastStopEmoteSound();

	USoundBase* SoundToPlay = EmoteDef->EmoteSound.LoadSynchronous();
	if (!SoundToPlay) return;

	if (GetNetMode() == NM_DedicatedServer) return;

	if (EmoteDef->bPlayAs3DSound)
	{
		ActiveEmoteAudioComponent = NewObject<UAudioComponent>(this);
		if (ActiveEmoteAudioComponent)
		{
			ActiveEmoteAudioComponent->SetSound(SoundToPlay);
			ActiveEmoteAudioComponent->bAllowSpatialization = true;
			ActiveEmoteAudioComponent->AttenuationOverrides.bAttenuate = true;
			ActiveEmoteAudioComponent->AttenuationOverrides.bSpatialize = true;
			ActiveEmoteAudioComponent->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
			ActiveEmoteAudioComponent->AttenuationOverrides.AttenuationShapeExtents = FVector(EmoteDef->SoundRadius * 0.1f, 0.f, 0.f);
			ActiveEmoteAudioComponent->AttenuationOverrides.FalloffDistance = EmoteDef->SoundRadius * 0.9f;
			ActiveEmoteAudioComponent->AdjustAttenuation(ActiveEmoteAudioComponent->AttenuationOverrides);

			ActiveEmoteAudioComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			ActiveEmoteAudioComponent->bAutoDestroy = true;
			ActiveEmoteAudioComponent->RegisterComponent();
			ActiveEmoteAudioComponent->Play();
		}

#if !UE_BUILD_SHIPPING
		if (EmoteDef->bShowSoundRadiusDebug)
		{
			DrawDebugSphere(
				GetWorld(),
				GetActorLocation(),
				EmoteDef->SoundRadius,
				32,
				FColor::Green,
				false,
				SoundToPlay->GetDuration(),
				0,
				1.5f
			);
		}
#endif
	}
	else
	{
		ActiveEmoteAudioComponent = UGameplayStatics::SpawnSound2D(
			GetWorld(),
			SoundToPlay,
			1.f,
			1.f,
			0.f,
			nullptr,
			true
		);
	}
}

void AGSPawn::MulticastStopEmoteSound_Implementation()
{
	if (ActiveEmoteAudioComponent)
	{
		ActiveEmoteAudioComponent->Stop();
		ActiveEmoteAudioComponent = nullptr;
	}
}

void AGSPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GSGameplayTags::State_Emoting))
	{
		if (!CachedMovementInput.IsZero() || bCachedJumpPressed)
		{
			FGameplayTagContainer EmotingTags;
			EmotingTags.AddTag(GSGameplayTags::State_Emoting);
			AbilitySystemComponent->CancelAbilities(&EmotingTags);
		}
	}

	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	FVector MoveDirection = FVector::ZeroVector;

	if (NavMoverComponent)
	{
		FVector NavMoveInputIntent = FVector::ZeroVector;
		FVector NavMoveInputVelocity = FVector::ZeroVector;
		if (NavMoverComponent->ConsumeNavMovementData(NavMoveInputIntent, NavMoveInputVelocity))
		{
			if (NavMoveInputIntent.IsNearlyZero() && !NavMoveInputVelocity.IsNearlyZero())
			{
				MoveDirection = NavMoveInputVelocity.GetSafeNormal();
			}
			else
			{
				MoveDirection = NavMoveInputIntent;
			}
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
