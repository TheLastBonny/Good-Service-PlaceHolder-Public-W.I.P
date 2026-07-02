#include "Abilities/GSAbility_Launch.h"
#include "Components/GSGrabbableComponent.h"
#include "Items/GSItem.h"
#include "Characters/GSPlayerController.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"

UGSAbility_Launch::UGSAbility_Launch()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	HoldThreshold = 0.3f;
	BaseThrowForce = 800.0f;
	MinThrowDistance = 50.0f;
	MaxThrowDistance = 600.0f;
	DropForwardOffset = 90.0f;
	ActivationTime = 0.0f;

	bShowDebugShape = false;
	DebugColor = FColor::Orange;
	DebugLifeTime = 2.0f;
}

void UGSAbility_Launch::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	FString NetRole = ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch::ActivateAbility started."), *NetRole);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: CommitAbility failed! Ending ability."), *NetRole);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!AvatarActor || !ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: AvatarActor or ASC is NULL! Ending ability."), *NetRole);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* HeldItem = nullptr;
	UGSGrabbableComponent* GrabComp = nullptr;
	AGSPlayerController* PC = Cast<AGSPlayerController>(ActorInfo->PlayerController.Get());
	if (PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: PC found. PC->LastGrabbedActor = %s"), 
			*NetRole, PC->LastGrabbedActor ? *PC->LastGrabbedActor->GetName() : TEXT("NULL"));
		if (PC->LastGrabbedActor)
		{
			HeldItem = PC->LastGrabbedActor;
			GrabComp = HeldItem->FindComponentByClass<UGSGrabbableComponent>();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: PlayerController was NULL!"), *NetRole);
	}

	if (!HeldItem || !GrabComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: HeldItem or GrabComp is NULL! Ending ability. HeldItem = %s, GrabComp = %s"), 
			*NetRole, HeldItem ? *HeldItem->GetName() : TEXT("NULL"), GrabComp ? *GrabComp->GetName() : TEXT("NULL"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (PC)
	{
		PC->ShowAimCursor();
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Called ShowAimCursor on PC."), *NetRole);
	}

	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Aiming mode started. Adding tag State.Aiming."), *NetRole);
	ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Aiming")), 1);
	ActivationTime = GetWorld()->GetTimeSeconds();
}

void UGSAbility_Launch::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	FString NetRole = ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch::InputReleased override called."), *NetRole);

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!AvatarActor || !ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: InputReleased - AvatarActor or ASC is NULL. Ending ability."), *NetRole);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGSPlayerController* PC = Cast<AGSPlayerController>(ActorInfo->PlayerController.Get());
	if (PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Calling HideAimCursor on PC."), *NetRole);
		PC->HideAimCursor();
	}

	float HeldTime = GetWorld()->GetTimeSeconds() - ActivationTime;
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: InputReleased logic fired. HeldTime = %f (Threshold: %f)"), *NetRole, HeldTime, HoldThreshold);

	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Removing tag State.Aiming."), *NetRole);
	ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Aiming")), 1);

	AActor* HeldItem = nullptr;
	UGSGrabbableComponent* GrabComp = nullptr;
	if (PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Fetching item from PC->LastGrabbedActor: %s"), 
			*NetRole, PC->LastGrabbedActor ? *PC->LastGrabbedActor->GetName() : TEXT("NULL"));
		if (PC->LastGrabbedActor)
		{
			HeldItem = PC->LastGrabbedActor;
			GrabComp = HeldItem->FindComponentByClass<UGSGrabbableComponent>();
		}
	}

	if (HeldItem && GrabComp)
	{
		HeldItem->SetInstigator(Cast<APawn>(AvatarActor));

		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Detaching and releasing item: %s. Previous IsGrabbed: %d"), 
			*NetRole, *HeldItem->GetName(), GrabComp->IsGrabbed());
		
		GrabComp->SetGrabbed(false);

		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Removing State.HoldingItem loose tag from ASC."), *NetRole);
		ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem")), 1);

		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Detaching %s from actor."), *NetRole, *HeldItem->GetName());
		HeldItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Enabling actor collision on %s."), *NetRole, *HeldItem->GetName());
		HeldItem->SetActorEnableCollision(true);

		UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(HeldItem->GetRootComponent());
		if (!PrimitiveRoot)
		{
			PrimitiveRoot = HeldItem->FindComponentByClass<UPrimitiveComponent>();
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Fallback used to find Primitive Component: %s"), 
				*NetRole, PrimitiveRoot ? *PrimitiveRoot->GetName() : TEXT("NULL"));
		}

		if (PrimitiveRoot)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Restoring collision profile (%s) and enabled (%d) on PrimitiveRoot %s"), 
				*NetRole, *GrabComp->OriginalCollisionProfileName.ToString(), (int32)GrabComp->OriginalCollisionEnabled.GetValue(), *PrimitiveRoot->GetName());
			PrimitiveRoot->SetCollisionProfileName(GrabComp->OriginalCollisionProfileName);
			PrimitiveRoot->SetCollisionEnabled(GrabComp->OriginalCollisionEnabled);
		}

		if (HeldTime < HoldThreshold)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Drop executed (HeldTime < HoldThreshold)."), *NetRole);

			FVector LaunchStartLoc = HeldItem->GetActorLocation();
			FVector DropLoc = AvatarActor->GetActorLocation() + (AvatarActor->GetActorForwardVector() * DropForwardOffset);
			
			// Trace down to find the ground or nearest surface
			FVector TraceStart = DropLoc + FVector(0.0f, 0.0f, 100.0f);
			FVector TraceEnd = DropLoc - FVector(0.0f, 0.0f, 500.0f);
			FHitResult DropHit;
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(AvatarActor);
			TraceParams.AddIgnoredActor(HeldItem);

			if (GetWorld()->LineTraceSingleByChannel(DropHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
			{
				DropLoc.Z = DropHit.Location.Z + 10.0f; // Add a small offset to prevent clipping
			}
			else
			{
				// Fallback to ground level relative to character
				DropLoc.Z = AvatarActor->GetActorLocation().Z - 90.0f;
			}

			if (GrabComp)
			{
				GrabComp->LaunchKinematic(LaunchStartLoc, DropLoc, 0.0f, 0.2f);
			}
			else if (ActorInfo->IsNetAuthority())
			{
				HeldItem->SetActorLocation(DropLoc);
			}
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Item location set to: %s"), *NetRole, *DropLoc.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Throw executed (HeldTime >= HoldThreshold)."), *NetRole);

			if (GrabComp->ThrowMontage && ActorInfo->AnimInstance.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Playing throw montage."), *NetRole);
				ActorInfo->AnimInstance->Montage_Play(GrabComp->ThrowMontage);
			}

			FVector LaunchDirection = AvatarActor->GetActorForwardVector();
			float LaunchZ = 300.0f;

			if (PC)
			{
				FVector TargetLoc = PC->LastAimTargetLocation;
				UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Using Target Location from PC: %s"), *NetRole, *TargetLoc.ToString());

				FVector Dir = TargetLoc - AvatarActor->GetActorLocation();
				Dir.Z = 0.0f;
				
				float Distance = Dir.Size();
				if (Distance > 0.0f)
				{
					Dir.Normalize();
					LaunchDirection = Dir;
				}

				float ClampedDistance = FMath::Clamp(Distance, MinThrowDistance, MaxThrowDistance);
				float SpeedFraction = ClampedDistance / MaxThrowDistance;
				float ScaledSpeed = FMath::Lerp(GrabComp->ThrowSpeed * 0.4f, GrabComp->ThrowSpeed, SpeedFraction);

				LaunchZ = GrabComp->ThrowArcHeight;

				FVector LaunchStartLoc = AvatarActor->GetActorLocation() + (LaunchDirection * DropForwardOffset) + FVector(0.0f, 0.0f, 30.0f);
				
				if (GrabComp)
				{
					float HorizontalDistance = FVector::Dist(LaunchStartLoc, TargetLoc);
					// Scale the duration using the estimated 3D path length and the distance-scaled speed
					float EstimatedPathLength = HorizontalDistance + (1.5f * LaunchZ);
					float ThrowDuration = EstimatedPathLength / ScaledSpeed;
					if (ThrowDuration < 0.2f) ThrowDuration = 0.2f;
					if (ThrowDuration > 3.0f) ThrowDuration = 3.0f;

					GrabComp->LaunchKinematic(LaunchStartLoc, TargetLoc, LaunchZ, ThrowDuration);
				}
				else if (ActorInfo->IsNetAuthority())
				{
					HeldItem->SetActorLocation(LaunchStartLoc);
				}
			}
			else
			{
				FVector LaunchStartLoc = AvatarActor->GetActorLocation() + (LaunchDirection * DropForwardOffset) + FVector(0.0f, 0.0f, 30.0f);
				LaunchZ = GrabComp->ThrowArcHeight;
				
				if (GrabComp)
				{
					FVector TargetLoc = LaunchStartLoc + (LaunchDirection * MaxThrowDistance);
					float EstimatedPathLength = MaxThrowDistance + (1.5f * LaunchZ);
					float ThrowDuration = EstimatedPathLength / GrabComp->ThrowSpeed;
					if (ThrowDuration < 0.2f) ThrowDuration = 0.2f;
					if (ThrowDuration > 3.0f) ThrowDuration = 3.0f;

					GrabComp->LaunchKinematic(LaunchStartLoc, TargetLoc, LaunchZ, ThrowDuration);
				}
				else if (ActorInfo->IsNetAuthority())
				{
					HeldItem->SetActorLocation(LaunchStartLoc);
				}
			}
		}

		if (PC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: Cleaning up LastGrabbedActor on PC locally (setting to NULL)."), *NetRole);
			PC->LastGrabbedActor = nullptr;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ANTIGRAVITY_LOG][%s] UGSAbility_Launch: HeldItem or GrabComp was NULL in InputReleased! HeldItem = %s, GrabComp = %s"), 
			*NetRole, HeldItem ? *HeldItem->GetName() : TEXT("NULL"), GrabComp ? *GrabComp->GetName() : TEXT("NULL"));
	}

	if (ActorInfo->IsNetAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][SERVER] UGSAbility_Launch: Calling EndAbility on Server (Authority)."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][CLIENT] UGSAbility_Launch: Ending locally on Client (No replication)."));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
	}
}
