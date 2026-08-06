#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "DataAssets/GSStationDataAsset.h"
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





	UFUNCTION(BlueprintImplementableEvent, Category = "Station|Events")
	void OnItemAddedToStation(AActor* Item);

	UFUNCTION(BlueprintImplementableEvent, Category = "Station|Events")
	void OnItemRemovedFromStation(AActor* Item);

	UFUNCTION(BlueprintCallable, Category = "Station|GAS")
	void ApplyEffectToItem(AActor* Item, TSubclassOf<UGameplayEffect> EffectClass);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TArray<FName> StationSockets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FGameplayTagContainer AllowedItemTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool bHidePlacedItems = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool bLimitToSockets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugLogs = false;

	UFUNCTION(CallInEditor, Category = "Debug")
	void ToggleDebugLogs() { bShowDebugLogs = !bShowDebugLogs; }

	virtual void HandleItemAddedToStation(AActor* Item);

	virtual void HandleItemRemovedFromStation(AActor* Item);

	UFUNCTION(BlueprintImplementableEvent, Category = "Station|Events")
	void OnAttachedItemStateChanged(AActor* Item, FGameplayTag StateTag, bool bAdded);

	void OnItemStateTagChanged(const FGameplayTag Tag, int32 NewCount, FGameplayTag StateTag, TWeakObjectPtr<AActor> WeakItem);

	void UpdateEffectsForItem(AActor* Item);

	UFUNCTION()
	void OnPlacedItemGrabbed(AActor* GrabbedItem);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> StationVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	UPROPERTY(ReplicatedUsing = OnRep_PlacedItems, BlueprintReadOnly, Category = "Station")
	TArray<AActor*> PlacedItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGSStationDataAsset> StationData;

	TMap<AActor*, TArray<FActiveGameplayEffectHandle>> AppliedEffectsMap;

	UFUNCTION()
	void OnRep_PlacedItems(const TArray<AActor*>& OldPlacedItems);


	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	struct FGSItemSubscription
	{
		TMap<FGameplayTag, FDelegateHandle> TagHandles;
	};

	TMap<TWeakObjectPtr<AActor>, FGSItemSubscription> StationSubscriptions;


	TArray<FGSConditionalEffectEntry> ConditionalEffectsFromData;

	FName GetFirstFreeSocket() const;
};

