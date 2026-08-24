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

	UFUNCTION(BlueprintPure, Category = "Station")
	bool IsSocketOccupied(FName SocketName) const;

	UFUNCTION(BlueprintPure, Category = "Station")
	AActor* GetItemInSocket(FName SocketName) const;

	UFUNCTION(BlueprintCallable, Category = "Station")
	bool PlaceItemInSocket(AActor* Item, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Station")
	AActor* SpawnItemInSocket(TSubclassOf<AActor> ItemClass, FName SocketName);

	UFUNCTION(BlueprintPure, Category = "Station")
	FName GetFirstFreeSocket() const;

	UFUNCTION(BlueprintPure, Category = "Station")
	FName GetRandomFreeSocket() const;

	UFUNCTION(BlueprintPure, Category = "Station")
	FName GetFreeSocket() const;

	UFUNCTION(BlueprintPure, Category = "Station")
	TArray<FName> GetAllFreeSockets() const;

	UFUNCTION(BlueprintCallable, Category = "Station")
	void CacheMeshSockets();

	UFUNCTION(BlueprintPure, Category = "Station")
	const TArray<FName>& GetStationSockets() const { return StationSockets; }

	/** Called when an item enters the station. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Station|Events")
	void OnItemAddedToStation(AActor* Item);

	/** Called when an item is removed from the station. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Station|Events")
	void OnItemRemovedFromStation(AActor* Item);

	/** Applies a Gameplay Effect to the specified item. */
	UFUNCTION(BlueprintCallable, Category = "Station|GAS")
	void ApplyEffectToItem(AActor* Item, TSubclassOf<UGameplayEffect> EffectClass);

	UFUNCTION(BlueprintPure, Category = "Station")
	USceneComponent* FindAttachComponentForSocket(FName SocketName) const;

	UFUNCTION(CallInEditor, Category = "Config|Sockets", meta = (DisplayName = "Discover Sockets From Mesh"))
	void DiscoverSocketsFromMesh();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Sockets")
	TArray<FName> StationSockets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Sockets")
	bool bAutoDiscoverSockets = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Sockets", meta = (EditCondition = "bAutoDiscoverSockets"))
	FString SocketPrefixFilter = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Sockets")
	bool bRandomizeSocketPlacement = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FGameplayTagContainer AllowedItemTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bHidePlacedItems = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool bLimitToSockets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugLogs = false;

	UFUNCTION(CallInEditor, Category = "Debug")
	void ToggleDebugLogs() { bShowDebugLogs = !bShowDebugLogs; }

	/** Internal C++ handling when an item is added to the station. */
	virtual void HandleItemAddedToStation(AActor* Item);

	/** Internal C++ handling when an item is removed from the station. */
	virtual void HandleItemRemovedFromStation(AActor* Item);

	/** Called when any state tag changes on an attached item. */
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

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGSStationModule>> RuntimeModules;

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
};


