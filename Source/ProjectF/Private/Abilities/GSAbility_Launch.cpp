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
	DispersedThrowSpreadAngle = 30.0f;

	bShowDebugShape = false;
	DebugColor = FColor::Orange;
	DebugLifeTime = 2.0f;
}

void UGSAbility_Launch::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AActor* HeldItem = nullptr;
	UGSGrabbableComponent* GrabComp = nullptr;
	AGSPlayerController* PC = Cast<AGSPlayerController>(ActorInfo->PlayerController.Get());

	if (AvatarActor)
	{
		TArray<AActor*> AttachedActors;
		AvatarActor->GetAttachedActors(AttachedActors);
		AActor* TopHeldItem = nullptr;
		UGSGrabbableComponent* TopGrabComp = nullptr;
		float MaxZOffset = -1.0f;

		for (AActor* AttachedActor : AttachedActors)
		{
			if (AttachedActor)
			{
				UGSGrabbableComponent* TempGrabComp = AttachedActor->FindComponentByClass<UGSGrabbableComponent>();
				if (TempGrabComp && TempGrabComp->IsGrabbed())
				{
					float CurrentZ = AttachedActor->GetRootComponent() ? AttachedActor->GetRootComponent()->GetRelativeLocation().Z : 0.0f;
					if (CurrentZ > MaxZOffset)
					{
						MaxZOffset = CurrentZ;
						TopHeldItem = AttachedActor;
						TopGrabComp = TempGrabComp;
					}
				}
			}
		}

		if (TopHeldItem)
		{
			HeldItem = TopHeldItem;
			GrabComp = TopGrabComp;
			if (PC && PC->LastGrabbedActor != HeldItem)
			{
				PC->LastGrabbedActor = HeldItem;
				if (!PC->HasAuthority())
				{
					PC->Server_SetGrabbedActor(HeldItem);
				}
			}
		}
	}

	if (!HeldItem && PC)
	{
		if (PC->LastGrabbedActor)
		{
			HeldItem = PC->LastGrabbedActor;
			GrabComp = HeldItem->FindComponentByClass<UGSGrabbableComponent>();
		}
	}

	if (!HeldItem || !GrabComp)
	{


		if (!ActorInfo->IsNetAuthority())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	if (PC)
	{
		PC->ShowAimCursor();
		
	}

	
	ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Aiming")), 1);
	ActivationTime = GetWorld()->GetTimeSeconds();
}

void UGSAbility_Launch::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	FString NetRole = ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!AvatarActor || !ASC)
	{
		
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGSPlayerController* PC = Cast<AGSPlayerController>(ActorInfo->PlayerController.Get());
	if (PC)
	{
		
		PC->HideAimCursor();
	}

	float HeldTime = GetWorld()->GetTimeSeconds() - ActivationTime;
	
	

	
	ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Aiming")), 1);

	TArray<AActor*> GrabbedItems;
	TArray<UGSGrabbableComponent*> GrabComps;

	if (AvatarActor)
	{
		TArray<AActor*> AttachedActors;
		AvatarActor->GetAttachedActors(AttachedActors);
		for (AActor* AttachedActor : AttachedActors)
		{
			if (AttachedActor)
			{
				UGSGrabbableComponent* TempGrabComp = AttachedActor->FindComponentByClass<UGSGrabbableComponent>();
				if (TempGrabComp && TempGrabComp->IsGrabbed())
				{
					GrabbedItems.Add(AttachedActor);
					GrabComps.Add(TempGrabComp);
				}
			}
		}
	}

	for (int32 i = 0; i < GrabbedItems.Num() - 1; ++i)
	{
		for (int32 j = i + 1; j < GrabbedItems.Num(); ++j)
		{
			float Zi = GrabbedItems[i]->GetRootComponent() ? GrabbedItems[i]->GetRootComponent()->GetRelativeLocation().Z : 0.0f;
			float Zj = GrabbedItems[j]->GetRootComponent() ? GrabbedItems[j]->GetRootComponent()->GetRelativeLocation().Z : 0.0f;
			if (Zi > Zj)
			{
				GrabbedItems.Swap(i, j);
				GrabComps.Swap(i, j);
			}
		}
	}

	if (GrabbedItems.Num() == 0)
	{
		if (!ActorInfo->IsNetAuthority())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	bool bThrowAll = false;
	if (PC)
	{
		bThrowAll = PC->bLastReleaseWasSpecial;
	}

	TArray<AActor*> ItemsToThrow;
	TArray<UGSGrabbableComponent*> CompsToThrow;

	if (bThrowAll)
	{
		ItemsToThrow = GrabbedItems;
		CompsToThrow = GrabComps;
	}
	else if (GrabbedItems.Num() > 0)
	{
		ItemsToThrow.Add(GrabbedItems.Last());
		CompsToThrow.Add(GrabComps.Last());
	}

	int32 TotalItems = ItemsToThrow.Num();

	for (int32 i = 0; i < TotalItems; ++i)
	{
		AActor* HeldItem = ItemsToThrow[i];
		UGSGrabbableComponent* GrabComp = CompsToThrow[i];

		if (HeldItem && GrabComp)
		{
			HeldItem->SetInstigator(Cast<APawn>(AvatarActor));

			
			
			GrabComp->SetGrabbed(false);

			
			ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.HoldingItem")), 1);

			
			HeldItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			
			
			HeldItem->SetActorEnableCollision(true);

			UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(HeldItem->GetRootComponent());
			if (!PrimitiveRoot)
			{
				PrimitiveRoot = HeldItem->FindComponentByClass<UPrimitiveComponent>();
				
			}

			if (PrimitiveRoot)
			{
				
				PrimitiveRoot->SetCollisionProfileName(GrabComp->OriginalCollisionProfileName);
				PrimitiveRoot->SetCollisionEnabled(GrabComp->OriginalCollisionEnabled);
			}

			FVector BaseTargetLoc = PC ? PC->LastAimTargetLocation : (AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * MaxThrowDistance);
			FVector Dir = BaseTargetLoc - AvatarActor->GetActorLocation();
			Dir.Z = 0.0f;
			float Distance = Dir.Size();
			if (Distance > 0.0f)
			{
				Dir.Normalize();
			}
			else
			{
				Dir = AvatarActor->GetActorForwardVector();
			}
			float ClampedDistance = FMath::Clamp(Distance, MinThrowDistance, MaxThrowDistance);

			float SpreadAngle = DispersedThrowSpreadAngle;
			float Angle = 0.0f;
			if (TotalItems > 1)
			{
				Angle = -SpreadAngle / 2.0f + (i * (SpreadAngle / (TotalItems - 1)));
			}
			FVector DispersedDir = Dir.RotateAngleAxis(Angle, FVector::UpVector);
			FVector DispersedTarget = AvatarActor->GetActorLocation() + (DispersedDir * ClampedDistance);

			FVector LaunchStartLoc = AvatarActor->GetActorLocation() + (DispersedDir * DropForwardOffset) + FVector(0.0f, 0.0f, 30.0f);

			if (HeldTime < HoldThreshold)
			{
				FVector DropLoc = AvatarActor->GetActorLocation() + (DispersedDir * DropForwardOffset);
				FVector TraceStart = DropLoc + FVector(0.0f, 0.0f, 100.0f);
				FVector TraceEnd = DropLoc - FVector(0.0f, 0.0f, 500.0f);
				FHitResult DropHit;
				FCollisionQueryParams TraceParams;
				TraceParams.AddIgnoredActor(AvatarActor);
				for (AActor* ItemToIgnore : GrabbedItems)
				{
					TraceParams.AddIgnoredActor(ItemToIgnore);
				}

				if (GetWorld()->LineTraceSingleByChannel(DropHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
				{
					DropLoc.Z = DropHit.Location.Z + 10.0f;
				}
				else
				{
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
			}
			else
			{
				if (GrabComp->ThrowMontage && ActorInfo->AnimInstance.IsValid() && i == TotalItems - 1)
				{
					ActorInfo->AnimInstance->Montage_Play(GrabComp->ThrowMontage);
				}

				float ScaledSpeed = FMath::Lerp(GrabComp->ThrowSpeed * 0.4f, GrabComp->ThrowSpeed, ClampedDistance / MaxThrowDistance);
				float LaunchZ = GrabComp->ThrowArcHeight;
				float HorizontalDistance = FVector::Dist(LaunchStartLoc, DispersedTarget);
				float EstimatedPathLength = HorizontalDistance + (1.5f * LaunchZ);
				float ThrowDuration = EstimatedPathLength / ScaledSpeed;
				if (ThrowDuration < 0.2f) ThrowDuration = 0.2f;
				if (ThrowDuration > 3.0f) ThrowDuration = 3.0f;

				if (GrabComp)
				{
					GrabComp->LaunchKinematic(LaunchStartLoc, DispersedTarget, LaunchZ, ThrowDuration);
				}
				else if (ActorInfo->IsNetAuthority())
				{
					HeldItem->SetActorLocation(LaunchStartLoc);
				}
			}
		}
	}

	if (PC && !bThrowAll)
	{
		TArray<AActor*> RemainingAttached;
		AvatarActor->GetAttachedActors(RemainingAttached);
		AActor* NewTopItem = nullptr;
		float MaxZ = -1.0f;
		for (AActor* Attached : RemainingAttached)
		{
			if (Attached)
			{
				UGSGrabbableComponent* TempGrabComp = Attached->FindComponentByClass<UGSGrabbableComponent>();
				if (TempGrabComp && TempGrabComp->IsGrabbed())
				{
					float CurrentZ = Attached->GetRootComponent() ? Attached->GetRootComponent()->GetRelativeLocation().Z : 0.0f;
					if (CurrentZ > MaxZ)
					{
						MaxZ = CurrentZ;
						NewTopItem = Attached;
					}
				}
			}
		}
		PC->LastGrabbedActor = NewTopItem;
		if (NewTopItem && !PC->HasAuthority())
		{
			PC->Server_SetGrabbedActor(NewTopItem);
		}
	}
	else if (PC)
	{
		PC->LastGrabbedActor = nullptr;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, ActorInfo->IsNetAuthority(), false);
}
