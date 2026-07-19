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
#include "Machines/GSUtilityStation.h"
#include "EngineUtils.h"

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
	ReplicatedStackHeightOffset = 0.0f;
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


			if (OriginalCollisionEnabled == ECollisionEnabled::NoCollision)
			{
				OriginalCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
			}
			if (OriginalCollisionProfileName.IsNone())
			{
				OriginalCollisionProfileName = TEXT("BlockAllDynamic");
			}

			
		}
	}
}

void UGSGrabbableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (bIsFlying && Owner->GetAttachParentActor() != nullptr)
	{
		bIsFlying = false;
		if (Owner->HasAuthority())
		{
			KinematicFlightParams.bIsFlying = false;
			KinematicFlightParams.LaunchVelocity = FVector::ZeroVector;
			OnRep_KinematicFlightParams();
		}
		else
		{
			OnRep_KinematicFlightParams();
		}
		return;
	}


	if (bIsFlying && Owner->HasAuthority())
	{

		if (ProjectileComp && ProjectileComp->Velocity.IsNearlyZero(5.0f))
		{
			bIsFlying = false;
			KinematicFlightParams.bIsFlying = false;
			KinematicFlightParams.LaunchVelocity = FVector::ZeroVector;

			

			OnRep_KinematicFlightParams();
		}
	}
}

void UGSGrabbableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGSGrabbableComponent, bIsGrabbed);
	DOREPLIFETIME(UGSGrabbableComponent, KinematicFlightParams);
	DOREPLIFETIME(UGSGrabbableComponent, ReplicatedStackHeightOffset);
}

void UGSGrabbableComponent::OnRep_ReplicatedStackHeightOffset()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (Owner->GetAttachParentActor() && Owner->GetRootComponent())
	{
		FName SocketName = Owner->GetRootComponent()->GetAttachSocketName();
		if (SocketName.IsNone() || SocketName == TEXT("None"))
		{
			FTransform AboveHeadTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, ReplicatedStackHeightOffset));
			Owner->SetActorRelativeTransform(AboveHeadTransform);
		}
		else
		{
			FTransform StackTransform = RelativeTransform;
			FVector Loc = StackTransform.GetLocation();
			Loc.Z = ReplicatedStackHeightOffset;
			StackTransform.SetLocation(Loc);
			Owner->SetActorRelativeTransform(StackTransform);
		}
	}
}

void UGSGrabbableComponent::SetGrabbed(bool bInGrabbed)
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		FString NetRole = Owner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
		

		if (Owner->HasAuthority())
		{
			Owner->SetReplicateMovement(!bInGrabbed);
			
			if (bInGrabbed)
			{
				bIsFlying = false;
				KinematicFlightParams.bIsFlying = false;
				KinematicFlightParams.LaunchVelocity = FVector::ZeroVector;
				
				OnGrabbed.Broadcast(Owner);
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

	

	if (PrimitiveRoot)
	{
		if (bIsGrabbed)
		{
			bIsFlying = false;
			KinematicFlightParams.bIsFlying = false;
			KinematicFlightParams.LaunchVelocity = FVector::ZeroVector;


			Owner->SetActorHiddenInGame(false);


			UProjectileMovementComponent* TempProjComp = Owner->FindComponentByClass<UProjectileMovementComponent>();
			if (TempProjComp)
			{
				TempProjComp->Deactivate();
				TempProjComp->DestroyComponent();
			}
			ProjectileComp = nullptr;

			PrimitiveRoot->SetSimulatePhysics(false);
			PrimitiveRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);


			if (PrimitiveRoot != Owner->GetRootComponent())
			{
				PrimitiveRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
				
			}


			if (Owner->GetAttachParentActor() && Owner->GetRootComponent())
			{
				if (USceneComponent* ItemRoot = Owner->GetRootComponent())
				{
					ItemRoot->SetAbsolute(false, false, true);
				}

				FName SocketName = Owner->GetRootComponent()->GetAttachSocketName();
				if (SocketName.IsNone() || SocketName == TEXT("None"))
				{
					float Offset = ReplicatedStackHeightOffset > 0.0f ? ReplicatedStackHeightOffset : FallbackAboveHeadHeight;
					FTransform AboveHeadTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, Offset));
					Owner->SetActorRelativeTransform(AboveHeadTransform);
				}
				else
				{
					if (ReplicatedStackHeightOffset > 0.0f)
					{
						FTransform StackTransform = RelativeTransform;
						FVector Loc = StackTransform.GetLocation();
						Loc.Z = ReplicatedStackHeightOffset;
						StackTransform.SetLocation(Loc);
						Owner->SetActorRelativeTransform(StackTransform);
					}
					else
					{
						Owner->SetActorRelativeTransform(RelativeTransform);
					}
				}
			}

		}
		else
		{

			AActor* ParentActor = Owner->GetAttachParentActor();
			if (ParentActor && !ParentActor->IsA(AGSUtilityStation::StaticClass()))
			{
				Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			}

			Owner->SetActorEnableCollision(true);

			PrimitiveRoot->SetCollisionProfileName(OriginalCollisionProfileName);
			PrimitiveRoot->SetCollisionEnabled(OriginalCollisionEnabled);
			PrimitiveRoot->SetSimulatePhysics(false);


			if (!Owner->HasAuthority())
			{
				if (APlayerController* PC = Owner->GetWorld()->GetFirstPlayerController())
				{
					if (APawn* Pawn = PC->GetPawn())
					{
						if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pawn))
						{
							if (UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent())
							{
								ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem")));
							}
						}
					}
				}
			}
		}
	}
}

void UGSGrabbableComponent::LaunchKinematic(const FVector& InStartLoc, const FVector& InTargetLoc, float InArcHeight, float InDuration)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	FVector LaunchVelocity = FVector::ZeroVector;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner);
	if (Owner->GetInstigator())
	{
		ActorsToIgnore.Add(Owner->GetInstigator());
	}


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

		LaunchVelocity = (InTargetLoc - InStartLoc).GetSafeNormal() * ThrowSpeed;
		LaunchVelocity.Z += 200.0f;
	}

	if (Owner->HasAuthority())
	{

		KinematicFlightParams.bIsFlying = true;
		KinematicFlightParams.LaunchVelocity = LaunchVelocity;


		OnRep_KinematicFlightParams();
	}
	else if (Owner->GetNetMode() != NM_DedicatedServer)
	{

		bIsFlying = true;
		KinematicFlightParams.bIsFlying = true;
		KinematicFlightParams.LaunchVelocity = LaunchVelocity;
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
			PrimitiveRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			PrimitiveRoot->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
			PrimitiveRoot->SetSimulatePhysics(false);
		}


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

			if (PrimitiveRoot)
			{
				PrimitiveRoot->IgnoreActorWhenMoving(Owner, true);
				if (Owner->GetInstigator())
				{
					PrimitiveRoot->IgnoreActorWhenMoving(Owner->GetInstigator(), true);

					for (TActorIterator<AActor> It(GetWorld()); It; ++It)
					{
						AActor* OtherActor = *It;
						if (OtherActor && OtherActor != Owner && OtherActor->GetInstigator() == Owner->GetInstigator())
						{
							UGSGrabbableComponent* OtherGrab = OtherActor->FindComponentByClass<UGSGrabbableComponent>();
							if (OtherGrab && OtherGrab->bIsFlying)
							{
								PrimitiveRoot->IgnoreActorWhenMoving(OtherActor, true);
								UPrimitiveComponent* OtherRoot = Cast<UPrimitiveComponent>(OtherActor->GetRootComponent());
								if (OtherRoot)
								{
									OtherRoot->IgnoreActorWhenMoving(Owner, true);
								}
							}
						}
					}
				}
			}

			ProjectileComp->SetActive(true);
			ProjectileComp->Velocity = KinematicFlightParams.LaunchVelocity;
			ProjectileComp->Activate();
		}
	}
	else
	{

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
			PrimitiveRoot->IgnoreActorWhenMoving(Owner, false);
			if (Owner->GetInstigator())
			{
				PrimitiveRoot->IgnoreActorWhenMoving(Owner->GetInstigator(), false);

				for (TActorIterator<AActor> It(GetWorld()); It; ++It)
				{
					AActor* OtherActor = *It;
					if (OtherActor && OtherActor != Owner && OtherActor->GetInstigator() == Owner->GetInstigator())
					{
						PrimitiveRoot->IgnoreActorWhenMoving(OtherActor, false);
						UPrimitiveComponent* OtherRoot = Cast<UPrimitiveComponent>(OtherActor->GetRootComponent());
						if (OtherRoot)
						{
							OtherRoot->IgnoreActorWhenMoving(Owner, false);
						}
					}
				}
			}

			if (Owner->GetAttachParentActor())
			{
				if (Owner->HasAuthority())
				{
					Owner->SetActorRelativeLocation(FVector::ZeroVector);
					Owner->SetActorRelativeRotation(FRotator::ZeroRotator);
				}
				PrimitiveRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
			}
			else
			{
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
			}

			PrimitiveRoot->SetCollisionProfileName(OriginalCollisionProfileName);
			PrimitiveRoot->SetCollisionEnabled(OriginalCollisionEnabled);

			PrimitiveRoot->SetSimulatePhysics(false);
		}
	}
}
