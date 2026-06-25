// Copyright (c) 2026 Bonny. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "GSItemDataAsset.generated.h"

class UStaticMesh;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> MeshOverride = nullptr;
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
