
#include "DataAssets/GSItemDataAsset.h"
#include "Items/GSItem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"

static bool ShouldLogDebugForOwner(AActor* Owner)
{
	if (!Owner) return false;
	if (AGSItem* GSItem = Cast<AGSItem>(Owner))
	{
		return GSItem->ShouldShowDebugLogs();
	}
	return false;
}

void UGSItemStateAction_MeshOverride::Execute(AActor* Owner)
{
	bool bShowDebug = ShouldLogDebugForOwner(Owner);
	if (bShowDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MESH_DEBUG] UGSItemStateAction_MeshOverride::Execute on actor %s"), Owner ? *Owner->GetName() : TEXT("NULL"));
	}
	if (!Owner) return;
	
	if (!MeshOverride)
	{
		if (bShowDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MESH_DEBUG] MeshOverride is NULL on %s"), *Owner->GetName());
		}
		return;
	}

	UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (MeshComp)
	{
		MeshComp->SetStaticMesh(MeshOverride);
		if (bShowDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MESH_DEBUG] Static mesh successfully set to %s on %s"), *MeshOverride->GetName(), *Owner->GetName());
		}
	}
	else
	{
		if (bShowDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("[MESH_DEBUG] No UStaticMeshComponent found on %s!"), *Owner->GetName());
		}
	}
}

void UGSItemStateAction_PlaySound::Execute(AActor* Owner)
{
	if (!Owner || !SoundOverride) return;
	if (ShouldLogDebugForOwner(Owner))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SOUND_DEBUG] Playing sound %s on %s"), *SoundOverride->GetName(), *Owner->GetName());
	}
	if (Owner->GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::PlaySoundAtLocation(Owner, SoundOverride, Owner->GetActorLocation());
	}
}

void UGSItemStateAction_SpawnParticles::Execute(AActor* Owner)
{
	if (!Owner || !ParticleOverride) return;
	if (ShouldLogDebugForOwner(Owner))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PARTICLE_DEBUG] Spawning particles %s on %s"), *ParticleOverride->GetName(), *Owner->GetName());
	}
	if (Owner->GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::SpawnEmitterAtLocation(Owner->GetWorld(), ParticleOverride, Owner->GetActorLocation());
	}
}

UGSItemDataAsset::UGSItemDataAsset()
{
}

FGSItemStateDetails UGSItemDataAsset::GetStateDetails(FGameplayTag StateTag) const
{
	const FGSItemStateDetails* Details = ItemStatesMap.Find(StateTag);
	if (Details)
	{
		return *Details;
	}
	return FGSItemStateDetails();
}
