
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "GSItemDataAsset.generated.h"

class UStaticMesh;
class USoundBase;
class UParticleSystem;


UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTF_API UGSItemStateAction : public UObject
{
	GENERATED_BODY()

public:
	virtual void Execute(AActor* Owner) {}
};


UCLASS(BlueprintType, EditInlineNew, meta=(DisplayName="Mesh Override"))
class PROJECTF_API UGSItemStateAction_MeshOverride : public UGSItemStateAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> MeshOverride = nullptr;

	virtual void Execute(AActor* Owner) override;
};


UCLASS(BlueprintType, EditInlineNew, meta=(DisplayName="Play Sound"))
class PROJECTF_API UGSItemStateAction_PlaySound : public UGSItemStateAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> SoundOverride = nullptr;

	virtual void Execute(AActor* Owner) override;
};


UCLASS(BlueprintType, EditInlineNew, meta=(DisplayName="Spawn Particles"))
class PROJECTF_API UGSItemStateAction_SpawnParticles : public UGSItemStateAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UParticleSystem> ParticleOverride = nullptr;

	virtual void Execute(AActor* Owner) override;
};


USTRUCT(BlueprintType)
struct FGSItemStateDetails
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	FGameplayAttribute MaxProgressAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	float MaxProgressValue = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	FText StateName;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Actions")
	TArray<TObjectPtr<UGSItemStateAction>> Actions;
};

UCLASS(BlueprintType)
class PROJECTF_API UGSItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSItemDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Configuration")
	FGameplayTagContainer DefaultTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Configuration")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Configuration")
	TMap<FGameplayTag, FGSItemStateDetails> ItemStatesMap;

	UFUNCTION(BlueprintCallable, Category = "Item")
	FGSItemStateDetails GetStateDetails(FGameplayTag StateTag) const;
};
