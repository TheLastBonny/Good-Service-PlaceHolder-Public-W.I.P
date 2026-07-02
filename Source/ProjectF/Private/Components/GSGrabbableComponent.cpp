#include "Components/GSGrabbableComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/GameStateBase.h"
#include "AbilitySystemComponent.h"
#include "Characters/GSPlayerController.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Components/MeshComponent.h"

UGSGrabbableComponent::UGSGrabbableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);

	AttachmentSocketName = TEXT("HoldingSocket");
	RelativeTransform = FTransform::Identity;
	LaunchForceMultiplier = 1.0f;
	ThrowSpeed = 800.0f;
	ThrowArcHeight = 300.0f;
	FallbackAboveHeadHeight = 120.0f;
	bIsGrabbed = false;

	bIsFlying = false;

	bOriginalSimulatePhysics = false;
	OriginalCollisionProfileName = NAME_None;
	OriginalCollisionEnabled = ECollisionEnabled::QueryAndPhysics;

	ProjectileBounciness = 0.35f;
	ProjectileFriction = 0.4f;
	ProjectileGravityScale = 1.0f;
}

void UGSGrabbableComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		FString NetRole = Owner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
		if (Owner->HasAuthority())
		{
			Owner->SetReplicates(true);
			Owner->SetReplicateMovement(true);
		}

		UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (!PrimitiveRoot)
		{
			PrimitiveRoot = Owner->FindComponentByClass<UPrimitiveComponent>();
		}

		if (PrimitiveRoot)
		{
			bOriginalSimulatePhysics = PrimitiveRoot->IsSimulatingPhysics();
			OriginalCollisionProfileName = PrimitiveRoot->GetCollisionProfileName();
			OriginalCollisionEnabled = PrimitiveRoot->GetCollisionEnabled();

			// Safety fallback: if default is set to no collision, treat it as QueryAndPhysics
			if (OriginalCollisionEnabled == ECollisionEnabled::NoCollision)
			{
				OriginalCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
			}
			if (OriginalCollisionProfileName.IsNone())
			{
				OriginalCollisionProfileName = TEXT("BlockAllDynamic");
			}

			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSGrabbableComponent::BeginPlay: Cached settings for %s: Simulate: %d, Profile: %s, Enabled: %d"),
				*NetRole, *Owner->GetName(), bOriginalSimulatePhysics, *OriginalCollisionProfileName.ToString(), (int32)OriginalCollisionEnabled.GetValue());
		}
	}
}

void UGSGrabbableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Server-only gameplay-critical catch detection and settling detection
	if (bIsFlying && Owner->HasAuthority())
	{
		// 1. Check for overlapping catching characters (Server-authoritative)
		TArray<FOverlapResult> OverlapResults;
		FCollisionShape CollisionSphere = FCollisionShape::MakeSphere(100.0f); // 100 units catch radius
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Owner);
		if (Owner->GetInstigator())
		{
			QueryParams.AddIgnoredActor(Owner->GetInstigator());
		}

		FVector CurrentLocation = Owner->GetActorLocation();

		if (GetWorld()->OverlapMultiByChannel(OverlapResults, CurrentLocation, FQuat::Identity, ECC_Pawn, CollisionSphere, QueryParams))
		{
			for (const FOverlapResult& Overlap : OverlapResults)
			{
				AActor* OverlapActor = Overlap.GetActor();
				if (OverlapActor)
				{
					IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(OverlapActor);
					UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
					
					bool bHasCatchingTag = false;
					if (ASC)
					{
						bHasCatchingTag = ASC->HasMatchingGameplayTag(GSGameplayTags::State_Catching);
					}

					if (bHasCatchingTag)
					{
						APawn* OverlapPawn = Cast<APawn>(OverlapActor);
						AGSPlayerController* OverlapPC = OverlapPawn ? Cast<AGSPlayerController>(OverlapPawn->GetController()) : nullptr;
						if (OverlapPC)
						{
							bIsFlying = false;
							KinematicFlightParams.bIsFlying = false;
							KinematicFlightParams.LaunchVelocity = FVector::ZeroVector;

							// Deactivate projectile component immediately
							if (ProjectileComp)
							{
								ProjectileComp->Deactivate();
								ProjectileComp->Velocity = FVector::ZeroVector;
								ProjectileComp->SetActive(false);
							}

							Owner->SetReplicateMovement(true);
							OnRep_KinematicFlightParams();

							// Register this item as target for the hit player's grab ability
							OverlapPC->LastGrabbedActor = Owner;

							// Cancel existing grab ability (which acted as catching window) and activate the Grab ability
							FGameplayTagContainer GrabTags;
							GrabTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Grab")));
							
							ASC->CancelAbilities(&GrabTags);
							if (ASC->TryActivateAbilitiesByTag(GrabTags))
							{
								UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] Item %s was caught mid-flight by player %s!"), *Owner->GetName(), *OverlapActor->GetName());
								
								DrawDebugSphere(GetWorld(), OverlapActor->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f), 120.0f, 16, FColor::Green, false, 1.0f, 0, 2.0f);
								if (GEngine)
								{
									GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("¡%s ATRAPÓ %s!"), *OverlapActor->GetName(), *Owner->GetName()));
								}
								return;
							}
						}
					}
				}
			}
		}

		// 2. Check if the projectile has finished flying and settled on the ground
		if (ProjectileComp && ProjectileComp->Velocity.IsNearlyZero(5.0f))
		{
			bIsFlying = false;
			KinematicFlightParams.bIsFlying = false;
			KinematicFlightParams.LaunchVelocity = FVector::ZeroVector;

			UE_LOG(LogTemp, Log, TEXT("[ANTIGRAVITY_LOG][SERVER] Projectile settled for %s. Disabling flight flag."), *Owner->GetName());

			OnRep_KinematicFlightParams();
		}
	}
}

void UGSGrabbableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGSGrabbableComponent, bIsGrabbed);
	DOREPLIFETIME(UGSGrabbableComponent, KinematicFlightParams);
}

void UGSGrabbableComponent::SetGrabbed(bool bInGrabbed)
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		FString NetRole = Owner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSGrabbableComponent::SetGrabbed on %s: bInGrabbed = %d (Previous: %d)"),
			*NetRole, *Owner->GetName(), bInGrabbed, bIsGrabbed);

		if (Owner->HasAuthority())
		{
			Owner->SetReplicateMovement(!bInGrabbed);
			
			if (bInGrabbed)
			{
				bIsFlying = false;
				KinematicFlightParams.bIsFlying = false;
				KinematicFlightParams.LaunchVelocity = FVector::ZeroVector;
			}
		}
	}
	bIsGrabbed = bInGrabbed;
	OnRep_IsGrabbed();
}

bool UGSGrabbableComponent::IsGrabbed() const
{
	return bIsGrabbed;
}

void UGSGrabbableComponent::OnRep_IsGrabbed()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	FString NetRole = Owner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	if (!PrimitiveRoot)
	{
		PrimitiveRoot = Owner->FindComponentByClass<UPrimitiveComponent>();
	}

	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSGrabbableComponent::OnRep_IsGrabbed for %s: bIsGrabbed = %d, PrimitiveRoot = %s"),
		*NetRole, *Owner->GetName(), bIsGrabbed, PrimitiveRoot ? *PrimitiveRoot->GetName() : TEXT("NULL"));

	if (PrimitiveRoot)
	{
		if (bIsGrabbed)
		{
			bIsFlying = false;
			KinematicFlightParams.bIsFlying = false;
			KinematicFlightParams.LaunchVelocity = FVector::ZeroVector;

			// Destroy projectile component on grab
			UProjectileMovementComponent* TempProjComp = Owner->FindComponentByClass<UProjectileMovementComponent>();
			if (TempProjComp)
			{
				TempProjComp->Deactivate();
				TempProjComp->DestroyComponent();
			}
			ProjectileComp = nullptr;

			PrimitiveRoot->SetSimulatePhysics(false);
			PrimitiveRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			// Reset child primitive relative transform to zero to clear any projectile movement offsets!
			if (PrimitiveRoot != Owner->GetRootComponent())
			{
				PrimitiveRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG] UGSGrabbableComponent::OnRep_IsGrabbed: Reset child PrimitiveRoot relative offset to zero."));
			}

			// Sync fallback attachment relative transform
			if (Owner->GetAttachParentActor() && Owner->GetRootComponent())
			{
				if (USceneComponent* ItemRoot = Owner->GetRootComponent())
				{
					ItemRoot->SetAbsolute(false, false, true);
				}

				FName SocketName = Owner->GetRootComponent()->GetAttachSocketName();
				if (SocketName.IsNone() || SocketName == TEXT("None"))
				{
					FTransform AboveHeadTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, FallbackAboveHeadHeight));
					Owner->SetActorRelativeTransform(AboveHeadTransform);
				}
			}

		}
		else
		{
			Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			Owner->SetActorEnableCollision(true);

			PrimitiveRoot->SetCollisionProfileName(OriginalCollisionProfileName);
			PrimitiveRoot->SetCollisionEnabled(OriginalCollisionEnabled);
			PrimitiveRoot->SetSimulatePhysics(false); // Controlled by ProjectileMovementComponent instead
		}
	}
}

void UGSGrabbableComponent::LaunchKinematic(const FVector& InStartLoc, const FVector& InTargetLoc, float InArcHeight, float InDuration)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (Owner->HasAuthority())
	{
		FVector LaunchVelocity = FVector::ZeroVector;
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(Owner);
		if (Owner->GetInstigator())
		{
			ActorsToIgnore.Add(Owner->GetInstigator());
		}

		// 1. Calculate SuggestProjectileVelocity to guarantee it passes through the cursor target
		UGameplayStatics::FSuggestProjectileVelocityParameters Params(GetWorld(), InStartLoc, InTargetLoc, ThrowSpeed);
		Params.bFavorHighArc = false;
		Params.CollisionRadius = 0.0f;
		Params.OverrideGravityZ = 0.0f;
		Params.TraceOption = ESuggestProjVelocityTraceOption::DoNotTrace;
		Params.ActorsToIgnore = ActorsToIgnore;
		Params.bDrawDebug = false;

		bool bSuggested = UGameplayStatics::SuggestProjectileVelocity(Params, LaunchVelocity);

		if (!bSuggested)
		{
			// Fallback: direct vector at throw speed with a slight upward arc
			LaunchVelocity = (InTargetLoc - InStartLoc).GetSafeNormal() * ThrowSpeed;
			LaunchVelocity.Z += 200.0f;
		}

		// 2. Set replication details
		KinematicFlightParams.bIsFlying = true;
		KinematicFlightParams.LaunchVelocity = LaunchVelocity;

		// Trigger local launch on server
		OnRep_KinematicFlightParams();
	}
}

void UGSGrabbableComponent::OnRep_KinematicFlightParams()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	bIsFlying = KinematicFlightParams.bIsFlying;
	Owner->SetReplicateMovement(true);

	UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	if (!PrimitiveRoot)
	{
		PrimitiveRoot = Owner->FindComponentByClass<UPrimitiveComponent>();
	}

	if (bIsFlying)
	{
		Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Owner->SetActorEnableCollision(true);

		if (PrimitiveRoot)
		{
			PrimitiveRoot->SetCollisionProfileName(OriginalCollisionProfileName);
			PrimitiveRoot->SetCollisionEnabled(OriginalCollisionEnabled);
			PrimitiveRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			PrimitiveRoot->SetSimulatePhysics(false);
		}

		// Spawn the UProjectileMovementComponent dynamically for the flight duration
		if (!ProjectileComp)
		{
			ProjectileComp = NewObject<UProjectileMovementComponent>(Owner, TEXT("GSProjectileMovement"));
			if (ProjectileComp)
			{
				ProjectileComp->bShouldBounce = true;
				ProjectileComp->Bounciness = ProjectileBounciness;
				ProjectileComp->Friction = ProjectileFriction;
				ProjectileComp->ProjectileGravityScale = ProjectileGravityScale;
				ProjectileComp->bAutoActivate = false;
				ProjectileComp->SetUpdatedComponent(PrimitiveRoot);
				ProjectileComp->RegisterComponent();
			}
		}

		if (ProjectileComp)
		{
			// Ignore the owner actor and the throwing character to prevent self-collision on launch
			if (PrimitiveRoot)
			{
				PrimitiveRoot->IgnoreActorWhenMoving(Owner, true);
				if (Owner->GetInstigator())
				{
					PrimitiveRoot->IgnoreActorWhenMoving(Owner->GetInstigator(), true);
				}
			}

			ProjectileComp->SetActive(true);
			ProjectileComp->Velocity = KinematicFlightParams.LaunchVelocity;
			ProjectileComp->Activate();
		}
	}
	else
	{
		// Destroy Projectile Movement when flight ends
		UProjectileMovementComponent* TempProjComp = Owner->FindComponentByClass<UProjectileMovementComponent>();
		if (TempProjComp)
		{
			TempProjComp->Deactivate();
			TempProjComp->DestroyComponent();
		}
		ProjectileComp = nullptr;

		bool bIsCurrentlyGrabbed = IsGrabbed();
		if (PrimitiveRoot)
		{
			// Clear moving ignore actors when flight ends
			PrimitiveRoot->IgnoreActorWhenMoving(Owner, false);
			if (Owner->GetInstigator())
			{
				PrimitiveRoot->IgnoreActorWhenMoving(Owner->GetInstigator(), false);
			}

			// Teleport parent actor to the mesh landing location (Server-only) and reset mesh relative offset (Server & Client)
			if (PrimitiveRoot != Owner->GetRootComponent())
			{
				if (Owner->HasAuthority())
				{
					FVector LandedLocation = PrimitiveRoot->GetComponentLocation();
					FRotator LandedRotation = PrimitiveRoot->GetComponentRotation();
					Owner->SetActorLocationAndRotation(LandedLocation, LandedRotation, false, nullptr, ETeleportType::TeleportPhysics);
				}
				PrimitiveRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
			}

			PrimitiveRoot->SetCollisionProfileName(OriginalCollisionProfileName);
			PrimitiveRoot->SetCollisionEnabled(OriginalCollisionEnabled);
			// We do NOT simulate physics; we let the replicated movement smoothly position the settled item
			PrimitiveRoot->SetSimulatePhysics(false);
		}
	}
}
