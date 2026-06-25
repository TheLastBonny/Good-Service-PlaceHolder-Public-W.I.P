#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "GSUtilityStation.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UBoxComponent;
class UGameplayEffect;
class UGSStationDataAsset;


UCLASS()
class PROJECTF_API AGSUtilityStation : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSUtilityStation();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Funciones de consulta expuestas a Blueprints
	UFUNCTION(BlueprintPure, Category = "Station")
	AActor* GetLastPlacedItem() const;

	UFUNCTION(BlueprintCallable, Category = "Station")
	AActor* RemoveLastPlacedItem();

	UFUNCTION(BlueprintCallable, Category = "Station")
	AActor* RemovePlacedItem(AActor* Item);

	UFUNCTION(BlueprintPure, Category = "Station")
	const TArray<AActor*>& GetPlacedItems() const;

	UFUNCTION(BlueprintPure, Category = "Station")
	AActor* GetFirstReadyItem() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> StationVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Station")
	TArray<TObjectPtr<AActor>> PlacedItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGSStationDataAsset> StationData;

	TMap<TObjectPtr<AActor>, TArray<FActiveGameplayEffectHandle>> AppliedEffectsMap;


	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
