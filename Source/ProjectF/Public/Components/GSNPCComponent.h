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

public:
	// Event fired when NPC changes its state (e.g. moves from queue to table)
	UPROPERTY(BlueprintAssignable, Category = "NPC")
	FOnNPCStateChangedSignature OnNPCStateChanged;

	// Event fired when the NPC decides what to order
	UPROPERTY(BlueprintAssignable, Category = "NPC")
	FOnOrderChosenSignature OnOrderChosen;

	// Event fired when the order is successfully delivered or fails (e.g. patience timeout)
	UPROPERTY(BlueprintAssignable, Category = "NPC")
	FOnOrderFulfilledSignature OnOrderFulfilled;

	// Current status of the NPC
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|State")
	ENPCState CurrentNPCState;

	// The Menu data asset assigned to this level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Order")
	TObjectPtr<UGSMenuDataAsset> LevelMenu;

	// The C++ class of the money drop to spawn when the order is fulfilled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Order")
	TSubclassOf<class AGSMoneyItem> MoneyItemClass;

	// If true, delivered items must be cooked and not burned. If false, any raw food item matching the tag will be accepted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Order")
	bool bRequireCookedState;



	// The current recipe the NPC is waiting for
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Order")
	FGSFoodRecipeDetails ActiveOrder;

	// True if the NPC currently has an active order
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Order")
	bool bHasActiveOrder;

	// The target spot actor the NPC should move to (queue spot, table, exit, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Navigation")
	TObjectPtr<AActor> AssignedTargetSpot;

	// Transition the NPC to a new state and fire the delegate
	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	void SetNPCState(ENPCState NewState);

	// Selects a random order from the LevelMenu, sets ActiveOrder, and broadcasts the event
	UFUNCTION(BlueprintCallable, Category = "NPC|Order")
	void ChooseRandomOrder();

	// Checks if the given item matches the active order's tags and is correctly cooked
	UFUNCTION(BlueprintPure, Category = "NPC|Order")
	bool CheckIfItemMatchesOrder(AGSItem* Item) const;

	// Delivers the item to the NPC. Consumes the item and awards money if successful.
	UFUNCTION(BlueprintCallable, Category = "NPC|Order")
	bool DeliverItem(AGSItem* Item);
};
