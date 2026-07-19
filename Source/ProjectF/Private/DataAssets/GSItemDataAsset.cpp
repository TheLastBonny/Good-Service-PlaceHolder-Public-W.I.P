
#include "DataAssets/GSItemDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"

void UGSItemStateAction_MeshOverride::Execute(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("[MESH_DEBUG] UGSItemStateAction_MeshOverride::Execute ejecutado en el actor %s"), Owner ? *Owner->GetName() : TEXT("NULL"));
	if (!Owner) return;
	
	if (!MeshOverride)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MESH_DEBUG] MeshOverride es NULL en %s"), *Owner->GetName());
		return;
	}

	UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (MeshComp)
	{
		MeshComp->SetStaticMesh(MeshOverride);
		UE_LOG(LogTemp, Warning, TEXT("[MESH_DEBUG] Malla establecida con éxito a %s en %s"), *MeshOverride->GetName(), *Owner->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[MESH_DEBUG] ¡No se encontró ningún UStaticMeshComponent en %s!"), *Owner->GetName());
	}
}

void UGSItemStateAction_PlaySound::Execute(AActor* Owner)
{
	if (!Owner || !SoundOverride) return;
	UE_LOG(LogTemp, Warning, TEXT("[SOUND_DEBUG] Reproduciendo sonido %s en %s"), *SoundOverride->GetName(), *Owner->GetName());
	if (Owner->GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::PlaySoundAtLocation(Owner, SoundOverride, Owner->GetActorLocation());
	}
}

void UGSItemStateAction_SpawnParticles::Execute(AActor* Owner)
{
	if (!Owner || !ParticleOverride) return;
	UE_LOG(LogTemp, Warning, TEXT("[PARTICLE_DEBUG] Spawneando partículas %s en %s"), *ParticleOverride->GetName(), *Owner->GetName());
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
