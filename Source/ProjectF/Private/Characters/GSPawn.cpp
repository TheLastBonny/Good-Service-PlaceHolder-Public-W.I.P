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
#include "DataAssets/UGSCharacterDataAsset.h"
#include "Components/GSSkinComponent.h"
#include "Characters/GSPlayerController.h"
#include "Engine/OverlapResult.h"

AGSPawn::AGSPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CapsuleComponent->InitCapsuleSize(35.f, 90.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = false;
	RootComponent = CapsuleComponent;

	MoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("MoverComponent"));
	NavMoverComponent = CreateDefaultSubobject<UNavMoverComponent>(TEXT("NavMoverComponent"));
	SkinComponent = CreateDefaultSubobject<UGSSkinComponent>(TEXT("SkinComponent"));

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

	if (CharacterDataAsset)
	{
		ApplyCharacterDataAsset(CharacterDataAsset);
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

void AGSPawn::ApplyCharacterDataAsset(UGSCharacterDataAsset* DataAsset)
{
	if (!DataAsset || !AbilitySystemComponent)
	{
		return;
	}

	// 1. Override attributes dynamically
	for (const FGSAttributeOverride& AttributeOverride : DataAsset->AttributesToSet)
	{
		if (AttributeOverride.Attribute.IsValid())
		{
			AbilitySystemComponent->SetNumericAttributeBase(AttributeOverride.Attribute, AttributeOverride.Value);
		}
	}

	// 2. Grant initial abilities and bind to input slot tags if specified
	for (const FGSAbilityGrant& AbilityGrant : DataAsset->AbilitiesToGrant)
	{
		if (AbilityGrant.AbilityClass)
		{
			FGameplayAbilitySpec AbilitySpec(AbilityGrant.AbilityClass, AbilityGrant.Level);
			FGameplayAbilitySpecHandle SpecHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);

			if (AbilityGrant.AbilityTag.IsValid() && SpecHandle.IsValid())
			{
				if (FGameplayAbilitySpec* GrantedSpec = AbilitySystemComponent->FindAbilitySpecFromHandle(SpecHandle))
				{
					GrantedSpec->GetDynamicSpecSourceTags().AddTag(AbilityGrant.AbilityTag);
				}
			}

			if (AbilityGrant.InputSlotTag.IsValid() && AbilityGrant.AbilityTag.IsValid())
			{
				AssignAbilityToSlot(AbilityGrant.InputSlotTag, AbilityGrant.AbilityTag);
			}
		}
	}

	// 3. Apply initial Gameplay Effects (buffs, passive traits, auras)
	for (const FGSGameplayEffectGrant& EffectGrant : DataAsset->EffectsToApply)
	{
		if (EffectGrant.EffectClass)
		{
			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
				EffectGrant.EffectClass,
				EffectGrant.Level,
				Context
			);

			if (SpecHandle.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// 4. Grant loose initial Gameplay Tags
	if (DataAsset->InitialCharacterTags.Num() > 0)
	{
		AbilitySystemComponent->AddLooseGameplayTags(DataAsset->InitialCharacterTags);
	}

	// 5. Apply skin texture if configured in DataAsset
	if (SkinComponent)
	{
		SkinComponent->MaterialSkinParameterName = DataAsset->MaterialSkinParameterName;
		SkinComponent->MaterialIndex = DataAsset->MaterialIndex;

		if (UTexture2D* SkinTex = DataAsset->GetRandomOrDefaultSkinTexture())
		{
			SkinComponent->ApplySkinTextureAsset(SkinTex);
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
			bool bMatchesTag = Spec.GetDynamicSpecSourceTags().HasTag(AbilityTag);
			if (!bMatchesTag && Spec.Ability)
			{
				bMatchesTag = Spec.Ability->GetAssetTags().HasTag(AbilityTag) || Spec.Ability->AbilityTags.HasTag(AbilityTag);
			}

			if (bMatchesTag)
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
			bool bMatchesTag = Spec.GetDynamicSpecSourceTags().HasTag(AbilityTag);
			if (!bMatchesTag && Spec.Ability)
			{
				bMatchesTag = Spec.Ability->GetAssetTags().HasTag(AbilityTag) || Spec.Ability->AbilityTags.HasTag(AbilityTag);
			}

			if (bMatchesTag)
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
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	FVector MoveDirection = FVector::ZeroVector;
	bool bIsNavigating = false;

	if (NavMoverComponent)
	{
		FVector NavMoveInputIntent = FVector::ZeroVector;
		FVector NavMoveInputVelocity = FVector::ZeroVector;
		if (NavMoverComponent->ConsumeNavMovementData(NavMoveInputIntent, NavMoveInputVelocity))
		{
			bIsNavigating = true;

			if (!NavMoveInputVelocity.IsNearlyZero())
			{
				CharacterInputs.SetMoveInput(EMoveInputType::Velocity, NavMoveInputVelocity);
				MoveDirection = NavMoveInputVelocity.GetSafeNormal();
			}
			else if (!NavMoveInputIntent.IsNearlyZero())
			{
				CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, NavMoveInputIntent.GetSafeNormal());
				MoveDirection = NavMoveInputIntent.GetSafeNormal();
			}
			MoveDirection.Z = 0.0f;
		}
	}

	if (!bIsNavigating && !CachedMovementInput.IsZero())
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
		MoveDirection.Z = 0.0f;
		MoveDirection.Normalize();

		CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
	}

	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GSGameplayTags::State_Emoting))
	{
		if (!MoveDirection.IsNearlyZero() || bCachedJumpPressed)
		{
			FGameplayTagContainer EmotingTags;
			EmotingTags.AddTag(GSGameplayTags::State_Emoting);
			AbilitySystemComponent->CancelAbilities(&EmotingTags);
		}
	}

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
		if (!MoveDirection.IsNearlyZero())
		{
			CharacterInputs.OrientationIntent = MoveDirection.GetSafeNormal();
		}
		else
		{
			CharacterInputs.OrientationIntent = GetActorForwardVector();
		}
	}

	CharacterInputs.ControlRotation = GetControlRotation();

	bCachedJumpJustPressed = false;
}
