#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DataAssets/GSMenuDataAsset.h"
#include "GSNPCComponent.generated.h"

UENUM(BlueprintType)
enum class ENPCState : uint8
{
	None,
	Entering,
	WaitingInQueue,
	Ordering,
	WaitingForFood,
	Eating,
	Leaving
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCStateChangedSignature, ENPCState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOrderChosenSignature, FGSFoodRecipeDetails, ChosenOrder);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOrderFulfilledSignature, bool, bSuccess);

class AGSItem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTF_API UGSNPCComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSNPCComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "NPC")
	FOnNPCStateChangedSignature OnNPCStateChanged;


	UPROPERTY(BlueprintAssignable, Category = "NPC")
	FOnOrderChosenSignature OnOrderChosen;


	UPROPERTY(BlueprintAssignable, Category = "NPC")
	FOnOrderFulfilledSignature OnOrderFulfilled;


	UPROPERTY(ReplicatedUsing = OnRep_CurrentNPCState, EditAnywhere, BlueprintReadWrite, Category = "NPC|State")
	ENPCState CurrentNPCState;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Order")
	TObjectPtr<UGSMenuDataAsset> LevelMenu;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Order")
	TSubclassOf<class AGSMoneyItem> MoneyItemClass;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Order")
	bool bRequireCookedState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|UI")
	TSubclassOf<class UUserWidget> OrderWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|UI")
	TObjectPtr<class UUserWidget> ActiveOrderWidgetInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|UI")
	TObjectPtr<class UGSBillboardWidgetComponent> OrderWidgetComponent;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveOrder, VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Order")
	FGSFoodRecipeDetails ActiveOrder;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Order")
	bool bHasActiveOrder;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Navigation")
	TObjectPtr<AActor> AssignedTargetSpot;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Timers")
	float TotalFoodWaitTime;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Timers")
	float FoodWaitStartTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Timers", meta = (ClampMin = "0.0"))
	float MinFoodWaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Timers", meta = (ClampMin = "0.0"))
	float MaxFoodWaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Timers", meta = (ClampMin = "0.0"))
	float MinEatingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Timers", meta = (ClampMin = "0.0"))
	float MaxEatingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Order", meta = (ClampMin = "0"))
	int32 OrderTimeoutPenalty;


	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	void SetNPCState(ENPCState NewState);


	UFUNCTION(BlueprintCallable, Category = "NPC|Order")
	void ChooseRandomOrder();


	UFUNCTION(BlueprintPure, Category = "NPC|Order")
	bool CheckIfItemMatchesOrder(AGSItem* Item) const;


	UFUNCTION(BlueprintCallable, Category = "NPC|Order")
	bool DeliverItem(AGSItem* Item);

	UFUNCTION(BlueprintCallable, Category = "NPC|Navigation")
	void SetAssignedTargetSpot(AActor* NewSpot);

	UFUNCTION(BlueprintCallable, Category = "NPC|UI")
	void CreateOrUpdateOrderWidget();

	UFUNCTION(BlueprintCallable, Category = "NPC|UI")
	void RemoveOrderWidget();

	UFUNCTION()
	void OnRep_CurrentNPCState();

	UFUNCTION()
	void OnRep_ActiveOrder();

private:
	void HandleFoodTimeout();
	void HandleEatingFinished();
	void HandleStateChangedServer(ENPCState OldState, ENPCState NewState);

	FTimerHandle FoodWaitTimerHandle;
	FTimerHandle EatingTimerHandle;

	float PendingMoneyValue;
};
