#include "Abilities/GSAbility_Grab.h"
#include "Components/GSGrabbableComponent.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

UGSAbility_Grab::UGSAbility_Grab()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	GrabRadius = 120.0f;
	GrabForwardOffset = 80.0f;
}

void UGSAbility_Grab::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
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

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AvatarActor);
	FCollisionShape Shape = FCollisionShape::MakeSphere(GrabRadius);
	FVector ScanLocation = AvatarActor->GetActorLocation() + (AvatarActor->GetActorForwardVector() * GrabForwardOffset);
	
	bool bHit = GetWorld()->OverlapMultiByChannel(Overlaps, ScanLocation, FQuat::Identity, ECC_WorldDynamic, Shape, Params);

	AActor* BestTarget = nullptr;
	float MinDist = TNumericLimits<float>::Max();
	UGSGrabbableComponent* BestGrabComp = nullptr;

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

	if (BestTarget && BestGrabComp)
	{
		BestGrabComp->SetGrabbed(true);

		if (BestGrabComp->GrabMontage && ActorInfo->AnimInstance.IsValid())
		{
			ActorInfo->AnimInstance->Montage_Play(BestGrabComp->GrabMontage);
		}

		UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(BestTarget->GetRootComponent());
		if (PrimitiveRoot)
		{
			PrimitiveRoot->SetSimulatePhysics(false);
			PrimitiveRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		USceneComponent* AttachParent = Cast<USceneComponent>(AvatarActor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		if (!AttachParent)
		{
			AttachParent = AvatarActor->GetRootComponent();
		}

		BestTarget->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, BestGrabComp->AttachmentSocketName);
		BestTarget->SetActorRelativeTransform(BestGrabComp->RelativeTransform);

		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem")), 1, EGameplayTagReplicationState::TagOnly);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
