#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GSSkinComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkinAppliedSignature, UTexture2D*, NewSkinTexture);

/**
 * Replicated component that handles real-time Minecraft skin dynamic material application,
 * desktop file loading, and multiplayer replication for Pawns, Characters, and NPCs.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTF_API UGSSkinComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGSSkinComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Parameter name in the material for the skin texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin|Config")
	FName MaterialSkinParameterName;

	/** Material slot index to apply the dynamic skin instance to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin|Config")
	int32 MaterialIndex;

	/** Delegate fired whenever a new skin texture is successfully applied */
	UPROPERTY(BlueprintAssignable, Category = "Skin")
	FOnSkinAppliedSignature OnSkinApplied;

	/** Opens desktop file dialog, imports selected skin PNG, and replicates to server */
	UFUNCTION(BlueprintCallable, Category = "Skin")
	bool OpenAndApplySkinFromFile();

	/** Applies a skin image directly from a local file path */
	UFUNCTION(BlueprintCallable, Category = "Skin")
	bool ApplySkinFromFilePath(const FString& FilePath);

	/** Applies a skin from a compressed image byte buffer (e.g. PNG bytes) */
	UFUNCTION(BlueprintCallable, Category = "Skin")
	bool ApplySkinFromBuffer(const TArray<uint8>& SkinBytes);

	/** Applies an existing UTexture2D asset directly (e.g. from DataAsset for NPCs) */
	UFUNCTION(BlueprintCallable, Category = "Skin")
	bool ApplySkinTextureAsset(UTexture2D* TextureAsset);

	/** Server RPC to upload skin buffer from local client */
	UFUNCTION(Server, Reliable, Category = "Skin")
	void Server_SetSkinBuffer(const TArray<uint8>& SkinBytes);

	/** Server RPC to set preset skin texture asset */
	UFUNCTION(Server, Reliable, Category = "Skin")
	void Server_SetSkinAsset(UTexture2D* TextureAsset);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_SkinBuffer)
	TArray<uint8> ReplicatedSkinBuffer;

	UPROPERTY(ReplicatedUsing = OnRep_SkinTextureAsset)
	TObjectPtr<UTexture2D> ReplicatedSkinTextureAsset;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CurrentActiveTexture;

	UFUNCTION()
	void OnRep_SkinBuffer();

	UFUNCTION()
	void OnRep_SkinTextureAsset();

	UMeshComponent* GetTargetMeshComponent() const;
	UMaterialInstanceDynamic* EnsureDynamicMaterial();
	void ApplyTextureInternal(UTexture2D* Texture);
};
