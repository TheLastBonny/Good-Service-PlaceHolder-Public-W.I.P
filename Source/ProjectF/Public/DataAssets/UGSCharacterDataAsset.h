#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "UGSCharacterDataAsset.generated.h"

/**
 * Struct representing an additive attribute override.
 * Allows setting any Gameplay Attribute's base value dynamically.
 */
USTRUCT(BlueprintType)
struct FGSAttributeOverride
{
	GENERATED_BODY()

	/** Target Gameplay Attribute to override (e.g. GSHealthAttributeSet.MaxHealth) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute")
	FGameplayAttribute Attribute;

	/** New base value for the attribute */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute")
	float Value = 100.f;
};

/**
 * Struct representing a Gameplay Ability grant with optional input slot binding.
 */
USTRUCT(BlueprintType)
struct FGSAbilityGrant
{
	GENERATED_BODY()

	/** Gameplay Ability class to grant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** Initial level of the granted ability */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	int32 Level = 1;

	/** Tag identifying the ability itself (e.g. Ability.Type.Grab) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FGameplayTag AbilityTag;

	/** Optional input slot tag to bind this ability to an action slot (e.g. Input.Slot.Action1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FGameplayTag InputSlotTag;
};

/**
 * Struct representing an initial or passive Gameplay Effect to apply on spawn.
 */
USTRUCT(BlueprintType)
struct FGSGameplayEffectGrant
{
	GENERATED_BODY()

	/** Gameplay Effect class to apply */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> EffectClass;

	/** Level for the applied Gameplay Effect */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float Level = 1.f;
};

/**
 * Modular Data Asset for character and NPC configuration.
 * Fully additive: only specified attributes, abilities, effects, and tags will be applied.
 */
UCLASS(BlueprintType)
class PROJECTF_API UGSCharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSCharacterDataAsset();

	/** Additive attribute overrides to apply to the character's Ability System Component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Attributes")
	TArray<FGSAttributeOverride> AttributesToSet;

	/** Initial abilities to grant to the character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Abilities")
	TArray<FGSAbilityGrant> AbilitiesToGrant;

	/** Initial Gameplay Effects (buffs, passive traits, auras) to apply on spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Gameplay Effects")
	TArray<FGSGameplayEffectGrant> EffectsToApply;

	/** Loose Gameplay Tags to grant to the character on spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Tags")
	FGameplayTagContainer InitialCharacterTags;

	/** Default skin texture to apply to the character or NPC */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "5. Visuals & Skins")
	TSoftObjectPtr<UTexture2D> DefaultSkinTexture;

	/** Pool of skin textures for random assignment (e.g. for NPCs) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "5. Visuals & Skins")
	TArray<TSoftObjectPtr<UTexture2D>> SkinTexturePool;

	/** Parameter name in the material for the skin texture */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "5. Visuals & Skins")
	FName MaterialSkinParameterName = FName("SkinTexture");

	/** Index of the material slot to apply the dynamic skin instance to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "5. Visuals & Skins")
	int32 MaterialIndex = 0;

	/** Gets a random skin from SkinTexturePool, or falls back to DefaultSkinTexture */
	UFUNCTION(BlueprintCallable, Category = "Visuals & Skins")
	UTexture2D* GetRandomOrDefaultSkinTexture() const;
};
