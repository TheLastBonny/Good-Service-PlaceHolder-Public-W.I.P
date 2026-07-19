
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GSStationDataAsset.generated.h"

class UGameplayEffect;
class UAttributeSet;

/** Un efecto condicional: se aplica al ítem cuando éste alcanza el StateTag especificado.
 *  Ejemplo: StateTag=State.Condition.Cook.Cooked, EffectsToApply=[GE_Burning]
 *  → el horno aplicará GE_Burning automáticamente cuando la dona se cocine. */
USTRUCT(BlueprintType)
struct FGSConditionalEffectEntry
{
	GENERATED_BODY()

	/** Tag de estado que dispara la aplicación de los efectos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	FGameplayTag StateTag;

	/** GEs que se aplican al ítem cuando StateTag se activa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;
};

USTRUCT(BlueprintType)
struct FGSStationDetails
{
	GENERATED_BODY()

	/** Efectos aplicados al ítem inmediatamente al entrar a la estación. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	/** Efectos condicionales: se aplican cuando el ítem alcanza un estado específico mientras está en la estación. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<FGSConditionalEffectEntry> ConditionalEffects;

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
