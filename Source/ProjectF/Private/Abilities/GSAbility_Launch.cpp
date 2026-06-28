#include "Abilities/GSAbility_Launch.h"
#include "Components/GSGrabbableComponent.h"
#include "Characters/GSPlayerController.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Animation/AnimInstance.h"

UGSAbility_Launch::UGSAbility_Launch()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	HoldThreshold = 0.3f;
	BaseThrowForce = 800.0f;
	MinThrowDistance = 50.0f;
	MaxThrowDistance = 600.0f;
	DropForwardOffset = 90.0f;
	ActivationTime = 0.0f;
}

void UGSAbility_Launch::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AActor* HeldItem = nullptr;
	UGSGrabbableComponent* GrabComp = nullptr;
	TArray<AActor*> AttachedActors;
	AvatarActor->GetAttachedActors(AttachedActors);

	for (AActor* Attached : AttachedActors)
	{
		UGSGrabbableComponent* Comp = Attached->FindComponentByClass<UGSGrabbableComponent>();
		if (Comp && Comp->IsGrabbed())
		{
			HeldItem = Attached;
			GrabComp = Comp;
			break;
		}
	}

	if (!HeldItem || !GrabComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Aiming")), 1, EGameplayTagReplicationState::TagOnly);
	ActivationTime = GetWorld()->GetTimeSeconds();

	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	if (ReleaseTask)
	{
		ReleaseTask->OnRelease.AddDynamic(this, &UGSAbility_Launch::OnInputReleased);
		ReleaseTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGSAbility_Launch::OnInputReleased(float HeldTime)
{
	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();

	if (!AvatarActor || !ASC)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Aiming")), 1, EGameplayTagReplicationState::TagOnly);

	AActor* HeldItem = nullptr;
	UGSGrabbableComponent* GrabComp = nullptr;
	TArray<AActor*> AttachedActors;
	AvatarActor->GetAttachedActors(AttachedActors);

	for (AActor* Attached : AttachedActors)
	{
		UGSGrabbableComponent* Comp = Attached->FindComponentByClass<UGSGrabbableComponent>();
		if (Comp && Comp->IsGrabbed())
		{
			HeldItem = Attached;
			GrabComp = Comp;
			break;
		}
	}

	if (HeldItem && GrabComp)
	{
		GrabComp->SetGrabbed(false);
		ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem")), 1, EGameplayTagReplicationState::TagOnly);

		HeldItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(HeldItem->GetRootComponent());
		if (PrimitiveRoot)
		{
			PrimitiveRoot->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			PrimitiveRoot->SetSimulatePhysics(true);
		}

		float ActualHeldTime = GetWorld()->GetTimeSeconds() - ActivationTime;

		if (ActualHeldTime < HoldThreshold)
		{
			FVector DropLoc = AvatarActor->GetActorLocation() + (AvatarActor->GetActorForwardVector() * DropForwardOffset);
			HeldItem->SetActorLocation(DropLoc);

			if (PrimitiveRoot)
			{
				PrimitiveRoot->AddImpulse(AvatarActor->GetActorForwardVector() * 100.0f, NAME_None, true);
			}
		}
		else
		{
			if (GrabComp->ThrowMontage && CurrentActorInfo->AnimInstance.IsValid())
			{
				CurrentActorInfo->AnimInstance->Montage_Play(GrabComp->ThrowMontage);
			}

			FVector LaunchDirection = AvatarActor->GetActorForwardVector();
			float LaunchZ = 300.0f;

			AGSPlayerController* PC = Cast<AGSPlayerController>(CurrentActorInfo->PlayerController.Get());
			if (PC)
			{
				FHitResult HitResult;
				if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
				{
					FVector TargetLoc = HitResult.Location;
					FVector Dir = TargetLoc - AvatarActor->GetActorLocation();
					Dir.Z = 0.0f;
					
					float Distance = Dir.Size();
					if (Distance > 0.0f)
					{
						Dir.Normalize();
						LaunchDirection = Dir;
					}

					float ClampedDistance = FMath::Clamp(Distance, MinThrowDistance, MaxThrowDistance);
					float HorizontalSpeed = (ClampedDistance / MaxThrowDistance) * BaseThrowForce;

					LaunchZ = PC->ThrowArcHeight;

					FVector LaunchVelocity = (LaunchDirection * HorizontalSpeed * GrabComp->LaunchForceMultiplier) + FVector(0.0f, 0.0f, LaunchZ);

					if (PrimitiveRoot)
					{
						PrimitiveRoot->AddImpulse(LaunchVelocity, NAME_None, true);
					}
				}
			}
			else
			{
				FVector LaunchVelocity = (LaunchDirection * BaseThrowForce * GrabComp->LaunchForceMultiplier) + FVector(0.0f, 0.0f, LaunchZ);
				if (PrimitiveRoot)
				{
					PrimitiveRoot->AddImpulse(LaunchVelocity, NAME_None, true);
				}
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
