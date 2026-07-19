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
#include "MoverSimulationTypes.h"
#include "MoveLibrary/MoverBlackboard.h"

UGSAbility_Grab::UGSAbility_Grab()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	GrabRadius = 120.0f;
	GrabForwardOffset = 80.0f;

	bShowDebugShape = false;
	DebugColor = FColor::Orange;
	DebugLifeTime = 2.0f;

	StackSpacingOffset = 10.0f;
	bUseDynamicBoundsHeight = true;
	DefaultItemHeight = 100.0f;
	InitialStackOffsetZ = 0.0f;

	ActivationOwnedTags.AddTag(GSGameplayTags::State_Catching);
}

void UGSAbility_Grab::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	FString NetRole = ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!AvatarActor || !ASC)
	{
		
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	int32 HeldCount = 0;
	TArray<AActor*> AttachedActors;
	AvatarActor->GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (AttachedActor)
		{
			UGSGrabbableComponent* TempGrabComp = AttachedActor->FindComponentByClass<UGSGrabbableComponent>();
			if (TempGrabComp && TempGrabComp->IsGrabbed())
			{
				HeldCount++;
			}
		}
	}

	int32 MaxGrabLimit = GetAbilityLevel();
	if (HeldCount >= MaxGrabLimit)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* BestTarget = nullptr;
	UGSGrabbableComponent* BestGrabComp = nullptr;
	AGSPlayerController* PC = Cast<AGSPlayerController>(ActorInfo->PlayerController.Get());

	if (PC && PC->LastGrabbedActor)
	{
		UGSGrabbableComponent* TempGrabComp = PC->LastGrabbedActor->FindComponentByClass<UGSGrabbableComponent>();
		if (TempGrabComp && !TempGrabComp->IsGrabbed())
		{
			BestTarget = PC->LastGrabbedActor;
			BestGrabComp = TempGrabComp;
		}
	}

	if (!BestTarget)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(AvatarActor);

		float Radius = GrabRadius;
		if (ActorInfo->IsNetAuthority())
		{
			Radius += 20.0f;
		}
		FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
		FVector ScanLocation = AvatarActor->GetActorLocation() + (AvatarActor->GetActorForwardVector() * GrabForwardOffset);

		if (!ActorInfo->IsNetAuthority() && bShowDebugShape)
		{
			DrawDebugSphere(GetWorld(), ScanLocation, GrabRadius, 32, DebugColor, false, DebugLifeTime, 0, 1.5f);
		}

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

		GetWorld()->OverlapMultiByObjectType(Overlaps, ScanLocation, FQuat::Identity, ObjectQueryParams, Shape, Params);

		float MinDist = TNumericLimits<float>::Max();
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Candidate = Overlap.GetActor();
			if (!Candidate) continue;

			UGSGrabbableComponent* GrabComp = Candidate->FindComponentByClass<UGSGrabbableComponent>();
			if (GrabComp && !GrabComp->IsGrabbed())
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

		if (BestTarget && PC)
		{
			PC->LastGrabbedActor = BestTarget;
			if (!ActorInfo->IsNetAuthority())
			{
				PC->Server_SetGrabbedActor(BestTarget);
			}
		}
	}

	if (BestTarget && BestGrabComp)
	{
		if (BestGrabComp->GrabMontage && ActorInfo->AnimInstance.IsValid())
		{
			ActorInfo->AnimInstance->Montage_Play(BestGrabComp->GrabMontage);
		}

		if (ActorInfo->IsNetAuthority())
		{
			BestGrabComp->SetGrabbed(true);

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
			}

			if (PrimitiveRoot)
			{
				PrimitiveRoot->SetSimulatePhysics(false);
				PrimitiveRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				if (PrimitiveRoot != BestTarget->GetRootComponent())
				{
					PrimitiveRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
				}
			}

			if (UCharacterMoverComponent* MoverComp = AvatarActor->FindComponentByClass<UCharacterMoverComponent>())
			{
				if (UMoverBlackboard* SimBlackboard = MoverComp->GetSimBlackboard_Mutable())
				{
					FRelativeBaseInfo EmptyBaseInfo;
					SimBlackboard->Set(CommonBlackboard::LastFoundDynamicMovementBase, EmptyBaseInfo);
				}
			}

			BestTarget->SetActorEnableCollision(false);
			BestTarget->SetActorHiddenInGame(false);

			USceneComponent* AttachParent = Cast<USceneComponent>(AvatarActor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
			if (!AttachParent)
			{
				AttachParent = AvatarActor->GetRootComponent();
			}

			if (USceneComponent* ItemRoot = BestTarget->GetRootComponent())
			{
				ItemRoot->SetAbsolute(false, false, true);
			}

			float CumulativeHeight = 0.0f;
			int32 StackCount = 0;
			TArray<AActor*> CurrAttached;
			AvatarActor->GetAttachedActors(CurrAttached);
			for (AActor* AttachedActor : CurrAttached)
			{
				if (AttachedActor && AttachedActor != BestTarget)
				{
					UGSGrabbableComponent* TempGrabComp = AttachedActor->FindComponentByClass<UGSGrabbableComponent>();
					if (TempGrabComp && TempGrabComp->IsGrabbed())
					{
						float ItemHeight = DefaultItemHeight;
						if (bUseDynamicBoundsHeight)
						{
							FVector Origin, BoxExtent;
							AttachedActor->GetActorBounds(false, Origin, BoxExtent);
							float CalculatedHeight = BoxExtent.Z * 2.0f;
							if (CalculatedHeight > 0.0f)
							{
								ItemHeight = CalculatedHeight;
							}
						}
						CumulativeHeight += ItemHeight;
						StackCount++;
					}
				}
			}

			float BaseOffset = 0.0f;
			bool bIsSocket = false;
			if (AttachParent && AttachParent->DoesSocketExist(BestGrabComp->AttachmentSocketName))
			{
				BaseOffset = BestGrabComp->RelativeTransform.GetLocation().Z;
				bIsSocket = true;
			}
			else
			{
				BaseOffset = BestGrabComp->FallbackAboveHeadHeight;
			}

			float CalculatedOffset = BaseOffset + InitialStackOffsetZ + CumulativeHeight + (StackCount * StackSpacingOffset);
			BestGrabComp->ReplicatedStackHeightOffset = CalculatedOffset;

			if (bIsSocket)
			{
				BestTarget->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, BestGrabComp->AttachmentSocketName);
				FTransform TargetTransform = BestGrabComp->RelativeTransform;
				FVector TargetLoc = TargetTransform.GetLocation();
				TargetLoc.Z = CalculatedOffset;
				TargetTransform.SetLocation(TargetLoc);
				BestTarget->SetActorRelativeTransform(TargetTransform);
			}
			else
			{
				BestTarget->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform);
				FTransform AboveHeadTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, CalculatedOffset));
				BestTarget->SetActorRelativeTransform(AboveHeadTransform);
			}

			ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem")), 1);
		}
		
		EndAbility(Handle, ActorInfo, ActivationInfo, ActorInfo->IsNetAuthority(), false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, ActorInfo->IsNetAuthority(), false);
	}
}
