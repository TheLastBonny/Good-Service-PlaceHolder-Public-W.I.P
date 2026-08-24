#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/GSNPCComponent.h"
#include "GameplayTagContainer.h"
#include "SmartObjectTypes.h"
#include "SmartObjectRuntime.h"
#include "DataAssets/GSMenuDataAsset.h"
#include "GSNPCComponentAlphaTest.generated.h"

class AGSItem;
class AGSMoneyItem;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlphaNPCStateChangedSignature, ENPCState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlphaOrderChosenSignature, FGSFoodRecipeDetails, ChosenOrder);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlphaOrderFulfilledSignature, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAlphaOrderWidgetUpdatedSignature, UUserWidget*, WidgetInstance, FGSFoodRecipeDetails, Recipe);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTF_API UGSNPCComponentAlphaTest : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSNPCComponentAlphaTest();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ==========================================
	// DELEGATES
	// ==========================================
	UPROPERTY(BlueprintAssignable, Category = "NPC Alpha | Events")
	FOnAlphaNPCStateChangedSignature OnNPCStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "NPC Alpha | Events")
	FOnAlphaOrderChosenSignature OnOrderChosen;

	UPROPERTY(BlueprintAssignable, Category = "NPC Alpha | Events")
	FOnAlphaOrderFulfilledSignature OnOrderFulfilled;

	UPROPERTY(BlueprintAssignable, Category = "NPC Alpha | Events")
	FOnAlphaOrderWidgetUpdatedSignature OnOrderWidgetUpdated;

	// ==========================================
	// CONFIGURATION & TAGS (EDITABLE IN EDITOR)
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Smart Objects")
	FGameplayTag ChairSmartObjectTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Smart Objects")
	FGameplayTag ExitSmartObjectTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Smart Objects")
	float SearchRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | State Flow")
	bool bAutoRunStateFlow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Order")
	TObjectPtr<UGSMenuDataAsset> LevelMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Order")
	TSubclassOf<AGSMoneyItem> MoneyItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Order")
	bool bRequireCookedState;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Timers")
	float TotalFoodWaitTime;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Timers")
	float FoodWaitStartTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Timers", meta = (ClampMin = "0.0"))
	float FoodWaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Timers", meta = (ClampMin = "0.0"))
	float EatingTime;

	// ==========================================
	// UI ORDER WIDGET EXPOSURE
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | UI")
	TSubclassOf<UUserWidget> OrderWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | UI")
	TObjectPtr<UUserWidget> ActiveOrderWidgetInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | UI")
	TObjectPtr<class UGSBillboardWidgetComponent> OrderWidgetComponent;

	// ==========================================
	// EVALUATION & INSPECTOR DEBUG
	// ==========================================
	UPROPERTY(ReplicatedUsing = OnRep_CurrentNPCState, VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Evaluation")
	ENPCState CurrentNPCState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Evaluation")
	FString CurrentStateDescription;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveOrder, VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Evaluation")
	FGSFoodRecipeDetails ActiveOrder;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Evaluation")
	bool bHasActiveOrder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Evaluation")
	TWeakObjectPtr<AActor> AssignedTargetSpot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Evaluation")
	FVector TargetLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Alpha | Evaluation")
	FVector CurrentNavTargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Alpha | Debug")
	bool bShowDebugLogs;

	FSmartObjectClaimHandle CurrentClaimHandle;

	// ==========================================
	// PUBLIC ACTION FUNCTIONS
	// ==========================================
	UFUNCTION(BlueprintCallable, Category = "NPC Alpha | Actions")
	void SetNPCState(ENPCState NewState);

	UFUNCTION(BlueprintCallable, Category = "NPC Alpha | Actions")
	bool ClaimAndMoveToSmartObject(FGameplayTag TargetTag);

	UFUNCTION(BlueprintCallable, Category = "NPC Alpha | Actions")
	void ReleaseCurrentSmartObject();

	UFUNCTION(BlueprintCallable, Category = "NPC Alpha | Actions")
	void ChooseRandomOrder();

	UFUNCTION(BlueprintPure, Category = "NPC Alpha | Actions")
	bool CheckIfItemMatchesOrder(AGSItem* Item) const;

	UFUNCTION(BlueprintCallable, Category = "NPC Alpha | Actions")
	bool DeliverItem(AGSItem* Item);

	UFUNCTION(BlueprintCallable, Category = "NPC Alpha | Actions")
	void ToggleDebugLogs() { bShowDebugLogs = !bShowDebugLogs; }

	UFUNCTION()
	void OnRep_CurrentNPCState();

	UFUNCTION()
	void OnRep_ActiveOrder();

	UFUNCTION()
	void OnCapsuleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	void StartWaitingForFoodTimer();
	void HandleFoodWaitTimeout();
	void StartEatingTimer();
	void HandleEatingFinished();
	void CreateOrUpdateOrderWidget();
	void RemoveOrderWidget();

	FTimerHandle FoodWaitTimerHandle;
	FTimerHandle EatingTimerHandle;
	FTimerHandle MovementPollTimerHandle;
	FTimerHandle InitialMoveRetryTimerHandle;

	float LastMoveRetryTime = 0.0f;
	float LastLogTime = 0.0f;
	bool bHasIssuedMoveRequest = false;

	void PollMovementToLocation();
	void RetryInitialMove();
};
