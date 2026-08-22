#include "Components/GSSkinComponent.h"
#include "Visuals/GSSkinLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

UGSSkinComponent::UGSSkinComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	MaterialSkinParameterName = FName("SkinTexture");
	MaterialIndex = 0;
	DynamicMaterialInstance = nullptr;
	CurrentActiveTexture = nullptr;
}

void UGSSkinComponent::BeginPlay()
{
	Super::BeginPlay();

	// Ensure MID is created on start if valid material exists
	EnsureDynamicMaterial();
}

void UGSSkinComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGSSkinComponent, ReplicatedSkinBuffer);
	DOREPLIFETIME(UGSSkinComponent, ReplicatedSkinTextureAsset);
}

UMeshComponent* UGSSkinComponent::GetTargetMeshComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	UMeshComponent* MeshComp = Cast<UMeshComponent>(Owner->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
	if (!MeshComp)
	{
		MeshComp = Cast<UMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	}

	return MeshComp;
}

UMaterialInstanceDynamic* UGSSkinComponent::EnsureDynamicMaterial()
{
	if (DynamicMaterialInstance)
	{
		return DynamicMaterialInstance;
	}

	UMeshComponent* MeshComp = GetTargetMeshComponent();
	if (!MeshComp)
	{
		return nullptr;
	}

	UMaterialInterface* BaseMat = MeshComp->GetMaterial(MaterialIndex);
	if (!BaseMat)
	{
		return nullptr;
	}

	DynamicMaterialInstance = MeshComp->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
	return DynamicMaterialInstance;
}

void UGSSkinComponent::ApplyTextureInternal(UTexture2D* Texture)
{
	if (!Texture)
	{
		return;
	}

	UMaterialInstanceDynamic* MID = EnsureDynamicMaterial();
	if (MID)
	{
		MID->SetTextureParameterValue(MaterialSkinParameterName, Texture);
		CurrentActiveTexture = Texture;
		OnSkinApplied.Broadcast(Texture);
	}
}

bool UGSSkinComponent::OpenAndApplySkinFromFile()
{
	FString SelectedPath;
	if (!UGSSkinLibrary::OpenSkinFileDialog(SelectedPath))
	{
		return false;
	}

	return ApplySkinFromFilePath(SelectedPath);
}

bool UGSSkinComponent::ApplySkinFromFilePath(const FString& FilePath)
{
	TArray<uint8> FileBytes;
	UTexture2D* ImportedTexture = UGSSkinLibrary::LoadTextureFromFile(FilePath, FileBytes);
	if (!ImportedTexture || FileBytes.Num() == 0)
	{
		return false;
	}

	// Apply locally immediately for zero-latency feedback
	ApplyTextureInternal(ImportedTexture);

	// Send buffer to server for multiplayer replication
	if (GetOwner() && GetOwner()->GetLocalRole() < ROLE_Authority)
	{
		Server_SetSkinBuffer(FileBytes);
	}
	else if (GetOwner() && GetOwner()->HasAuthority())
	{
		ReplicatedSkinBuffer = FileBytes;
		ReplicatedSkinTextureAsset = nullptr;
	}

	return true;
}

bool UGSSkinComponent::ApplySkinFromBuffer(const TArray<uint8>& SkinBytes)
{
	if (SkinBytes.Num() == 0)
	{
		return false;
	}

	UTexture2D* DecodedTexture = UGSSkinLibrary::LoadTextureFromBuffer(SkinBytes);
	if (!DecodedTexture)
	{
		return false;
	}

	ApplyTextureInternal(DecodedTexture);
	return true;
}

bool UGSSkinComponent::ApplySkinTextureAsset(UTexture2D* TextureAsset)
{
	if (!TextureAsset)
	{
		return false;
	}

	ApplyTextureInternal(TextureAsset);

	if (GetOwner() && GetOwner()->GetLocalRole() < ROLE_Authority)
	{
		Server_SetSkinAsset(TextureAsset);
	}
	else if (GetOwner() && GetOwner()->HasAuthority())
	{
		ReplicatedSkinTextureAsset = TextureAsset;
		ReplicatedSkinBuffer.Empty();
	}

	return true;
}

void UGSSkinComponent::Server_SetSkinBuffer_Implementation(const TArray<uint8>& SkinBytes)
{
	ReplicatedSkinBuffer = SkinBytes;
	ReplicatedSkinTextureAsset = nullptr;

	// Apply on server authority as well
	ApplySkinFromBuffer(ReplicatedSkinBuffer);
}

void UGSSkinComponent::Server_SetSkinAsset_Implementation(UTexture2D* TextureAsset)
{
	ReplicatedSkinTextureAsset = TextureAsset;
	ReplicatedSkinBuffer.Empty();

	// Apply on server authority
	ApplyTextureInternal(ReplicatedSkinTextureAsset);
}

void UGSSkinComponent::OnRep_SkinBuffer()
{
	if (ReplicatedSkinBuffer.Num() > 0)
	{
		ApplySkinFromBuffer(ReplicatedSkinBuffer);
	}
}

void UGSSkinComponent::OnRep_SkinTextureAsset()
{
	if (ReplicatedSkinTextureAsset)
	{
		ApplyTextureInternal(ReplicatedSkinTextureAsset);
	}
}
