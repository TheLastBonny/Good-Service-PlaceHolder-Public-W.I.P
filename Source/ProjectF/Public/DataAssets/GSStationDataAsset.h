// Copyright (c) 2026 Bonny. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSStationDataAsset.generated.h"

class UGameplayEffect;
class UAttributeSet;

USTRUCT(BlueprintType)
struct FGSStationDetails
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
	FVector HitBoxSize = FVector(100.0f, 100.0f, 100.0f);
};

UCLASS(BlueprintType)
class PROJECTF_API UGSStationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSStationDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Station Configuration")
	FGSStationDetails StationDetails;
};
