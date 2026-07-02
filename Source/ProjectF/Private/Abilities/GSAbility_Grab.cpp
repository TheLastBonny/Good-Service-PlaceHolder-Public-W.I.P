#include "Abilities/GSAbility_Grab.h"
#include "Components/GSGrabbableComponent.h"
#include "AbilitySystemComponent.h"
#include "Characters/GSPlayerController.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/MeshComponent.h"
#include "Core/GSGameplayTags.h"

UGSAbility_Grab::UGSAbility_Grab()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	GrabRadius = 120.0f;
	GrabForwardOffset = 80.0f;

	bShowDebugShape = false;
	DebugColor = FColor::Orange;
	DebugLifeTime = 2.0f;

	// Automatically grant Catching state while the grab ability is active
	ActivationOwnedTags.AddTag(GSGameplayTags::State_Catching);

	// Block activation if the player is already holding an item
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem")));
}

void UGSAbility_Grab::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	FString NetRole = ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab::ActivateAbility started. Avatar: %s, Controller: %s"), 
		*NetRole, ActorInfo->AvatarActor.IsValid() ? *ActorInfo->AvatarActor->GetName() : TEXT("NULL"),
		ActorInfo->PlayerController.IsValid() ? *ActorInfo->PlayerController->GetName() : TEXT("NULL"));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: CommitAbility failed! Ending ability."), *NetRole);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!AvatarActor || !ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: AvatarActor or ASC is NULL! Ending ability."), *NetRole);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Player is already holding an item! Cannot grab another."), *NetRole);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* BestTarget = nullptr;
	UGSGrabbableComponent* BestGrabComp = nullptr;
	AGSPlayerController* PC = Cast<AGSPlayerController>(ActorInfo->PlayerController.Get());

	// If we already have a pre-selected target (e.g. from a catch trigger), bypass the air check
	bool bHasPreselectedTarget = false;
	if (PC && PC->LastGrabbedActor)
	{
		bHasPreselectedTarget = true;
	}

	bool bIsInAir = false;
	if (!bHasPreselectedTarget)
	{
		if (UCharacterMoverComponent* Mover = AvatarActor->FindComponentByClass<UCharacterMoverComponent>())
		{
			bIsInAir = Mover->IsFalling();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: bIsInAir = %d, bHasPreselectedTarget = %d"), 
		*NetRole, bIsInAir, bHasPreselectedTarget);

	if (bIsInAir)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Player is in the air. Entering Catching window for 0.4 seconds..."), *NetRole);
		
		if (AvatarActor->GetWorld())
		{
			// Draw catching zone visual helper for 0.4 seconds
			DrawDebugSphere(AvatarActor->GetWorld(), AvatarActor->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f), 100.0f, 16, FColor::Cyan, false, 0.4f, 0, 1.0f);

			FTimerHandle CatchTimerHandle;
			AvatarActor->GetWorld()->GetTimerManager().SetTimer(
				CatchTimerHandle,
				[this, Handle, ActorInfo, ActivationInfo]()
				{
					if (ActorInfo->AvatarActor.IsValid())
					{
						FString CallbackRole = ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
						UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Jump-catching window expired. Ending ability."), *CallbackRole);
						EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
					}
				},
				0.4f,
				false
			);
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
		return;
	}

	if (ActorInfo->IsNetAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: Checking PC->LastGrabbedActor..."));
		if (PC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: PC->LastGrabbedActor = %s"), PC->LastGrabbedActor ? *PC->LastGrabbedActor->GetName() : TEXT("NULL"));
			if (PC->LastGrabbedActor)
			{
				UGSGrabbableComponent* TempGrabComp = PC->LastGrabbedActor->FindComponentByClass<UGSGrabbableComponent>();
				if (TempGrabComp)
				{
					UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: Found grabbable component. IsGrabbed: %d"), TempGrabComp->IsGrabbed());
					if (!TempGrabComp->IsGrabbed())
					{
						BestTarget = PC->LastGrabbedActor;
						BestGrabComp = TempGrabComp;
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: LastGrabbedActor has no UGSGrabbableComponent!"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: PlayerController was NULL on Authority!"));
		}

		if (!BestTarget)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: No pre-selected actor on PC. Running fallback scan on Server..."));
			TArray<FOverlapResult> Overlaps;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(AvatarActor);
			FCollisionShape Shape = FCollisionShape::MakeSphere(GrabRadius);
			FVector ScanLocation = AvatarActor->GetActorLocation() + (AvatarActor->GetActorForwardVector() * GrabForwardOffset);

			FCollisionObjectQueryParams ObjectQueryParams;
			ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

			GetWorld()->OverlapMultiByObjectType(Overlaps, ScanLocation, FQuat::Identity, ObjectQueryParams, Shape, Params);
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: Server fallback overlap scan found %d actors."), Overlaps.Num());

			float MinDist = TNumericLimits<float>::Max();
			for (const FOverlapResult& Overlap : Overlaps)
			{
				AActor* Candidate = Overlap.GetActor();
				if (!Candidate) continue;

				UGSGrabbableComponent* GrabComp = Candidate->FindComponentByClass<UGSGrabbableComponent>();
				if (GrabComp)
				{
					UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: Fallback candidate %s. IsGrabbed: %d"), *Candidate->GetName(), GrabComp->IsGrabbed());
					if (!GrabComp->IsGrabbed())
					{
						float Dist = FVector::DistSquared(AvatarActor->GetActorLocation(), Candidate->GetActorLocation());
						if (Dist < MinDist)
						{
							MinDist = Dist;
							BestTarget = Candidate;
							BestGrabComp = GrabComp;
						}
					}
				}
			}

			if (BestTarget && PC)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Grab: Server fallback registering target: %s"), *BestTarget->GetName());
				PC->LastGrabbedActor = BestTarget;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] UGSAbility_Grab: Running as Client. Performing overlap scan..."));
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(AvatarActor);
		FCollisionShape Shape = FCollisionShape::MakeSphere(GrabRadius);
		FVector ScanLocation = AvatarActor->GetActorLocation() + (AvatarActor->GetActorForwardVector() * GrabForwardOffset);

		if (bShowDebugShape)
		{
			DrawDebugSphere(GetWorld(), ScanLocation, GrabRadius, 32, DebugColor, false, DebugLifeTime, 0, 1.5f);
		}

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

		GetWorld()->OverlapMultiByObjectType(Overlaps, ScanLocation, FQuat::Identity, ObjectQueryParams, Shape, Params);
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] UGSAbility_Grab: Overlap scan found %d actors."), Overlaps.Num());

		float MinDist = TNumericLimits<float>::Max();
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Candidate = Overlap.GetActor();
			if (!Candidate) continue;

			UGSGrabbableComponent* GrabComp = Candidate->FindComponentByClass<UGSGrabbableComponent>();
			if (GrabComp)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] UGSAbility_Grab: Found candidate %s. IsGrabbed: %d"), *Candidate->GetName(), GrabComp->IsGrabbed());
				if (!GrabComp->IsGrabbed())
				{
					float Dist = FVector::DistSquared(AvatarActor->GetActorLocation(), Candidate->GetActorLocation());
					if (Dist < MinDist)
					{
						MinDist = Dist;
						BestTarget = Candidate;
						BestGrabComp = GrabComp;
					}
				}
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] UGSAbility_Grab: Client selected BestTarget: %s"), BestTarget ? *BestTarget->GetName() : TEXT("NONE"));

		if (BestTarget && PC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] UGSAbility_Grab: Client registering target on PC and sending RPC..."));
			PC->LastGrabbedActor = BestTarget;
			PC->Server_SetGrabbedActor(BestTarget);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Selected best target = %s, GrabComp = %s"), 
		*NetRole, BestTarget ? *BestTarget->GetName() : TEXT("NULL"), BestGrabComp ? *BestGrabComp->GetName() : TEXT("NULL"));

	if (BestTarget && BestGrabComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: BestGrabComp->IsGrabbed() = %d"), *NetRole, BestGrabComp->IsGrabbed());
		if (!BestGrabComp->IsGrabbed())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Executing grab on target: %s"), *NetRole, *BestTarget->GetName());
			BestGrabComp->SetGrabbed(true);

			if (BestGrabComp->GrabMontage && ActorInfo->AnimInstance.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Playing grab montage on AnimInstance."), *NetRole);
				ActorInfo->AnimInstance->Montage_Play(BestGrabComp->GrabMontage);
			}

			// Destroy projectile component immediately on grab to prevent movement during attachment
			UProjectileMovementComponent* TargetProjComp = BestTarget->FindComponentByClass<UProjectileMovementComponent>();
			if (TargetProjComp)
			{
				TargetProjComp->Deactivate();
				TargetProjComp->DestroyComponent();
			}

			UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(BestTarget->GetRootComponent());
			if (!PrimitiveRoot)
			{
				PrimitiveRoot = BestTarget->FindComponentByClass<UPrimitiveComponent>();
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Fallback used to find Primitive Component: %s"), 
					*NetRole, PrimitiveRoot ? *PrimitiveRoot->GetName() : TEXT("NULL"));
			}

			if (PrimitiveRoot)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Disabling physics and collision on PrimitiveRoot %s"), *NetRole, *PrimitiveRoot->GetName());
				PrimitiveRoot->SetSimulatePhysics(false);
				PrimitiveRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				// Reset child primitive relative transform to zero to clear any projectile movement offsets!
				if (PrimitiveRoot != BestTarget->GetRootComponent())
				{
					PrimitiveRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Disabling actor collision on %s"), *NetRole, *BestTarget->GetName());
			BestTarget->SetActorEnableCollision(false);

			USceneComponent* AttachParent = Cast<USceneComponent>(AvatarActor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
			if (!AttachParent)
			{
				AttachParent = AvatarActor->GetRootComponent();
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Fallback to root component for attachment: %s"), *NetRole, *AttachParent->GetName());
			}

			// Prevent the item's root component from inheriting the parent's scale
			if (USceneComponent* ItemRoot = BestTarget->GetRootComponent())
			{
				ItemRoot->SetAbsolute(false, false, true);
			}

			if (AttachParent && AttachParent->DoesSocketExist(BestGrabComp->AttachmentSocketName))
			{
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Attaching to socket %s"), *NetRole, *BestGrabComp->AttachmentSocketName.ToString());
				BestTarget->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, BestGrabComp->AttachmentSocketName);
				BestTarget->SetActorRelativeTransform(BestGrabComp->RelativeTransform);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Socket %s not found. Attaching to parent root as fallback."), 
					*NetRole, *BestGrabComp->AttachmentSocketName.ToString());
				BestTarget->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform);
				float HeightOffset = BestGrabComp->FallbackAboveHeadHeight;
				FTransform AboveHeadTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, HeightOffset));
				BestTarget->SetActorRelativeTransform(AboveHeadTransform);
			}





			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Adding State.HoldingItem loose tag to ASC."), *NetRole);
			ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem")), 1);
		}
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: Ending Grab Ability after successful grab."), *NetRole);
		if (ActorInfo->IsNetAuthority())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Grab: No target found on ground. Ending Grab Ability immediately."), *NetRole);
		if (ActorInfo->IsNetAuthority())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		}
	}
}
