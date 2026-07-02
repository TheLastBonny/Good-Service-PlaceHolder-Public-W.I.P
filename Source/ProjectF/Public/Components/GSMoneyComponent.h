#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSMoneyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComponentOnMoneyChangedSignature, int32, NewMoney);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComponentOnMoneyDepositedSignature, class AGSMoneyItem*, MoneyItem, int32, FinalAddedValue);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTF_API UGSMoneyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSMoneyComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Event fired when global money changes
	UPROPERTY(BlueprintAssignable, Category = "Money")
	FComponentOnMoneyChangedSignature OnMoneyChanged;

	// Event fired locally when this component consumes a money drop (e.g. Cash Register behavior)
	UPROPERTY(BlueprintAssignable, Category = "Money|Register")
	FComponentOnMoneyDepositedSignature OnMoneyDeposited;

	// Returns the current global money amount
	UFUNCTION(BlueprintPure, Category = "Money")
	int32 GetCurrentMoney() const;

	// Returns the global money multiplier
	UFUNCTION(BlueprintPure, Category = "Money")
	float GetGlobalMoneyMultiplier() const;

	// Adds money directly to the GameState (applying global multipliers)
	UFUNCTION(BlueprintCallable, Category = "Money")
	void AddMoney(int32 Amount);

	// Removes money directly from the GameState. Returns false if not enough funds.
	UFUNCTION(BlueprintCallable, Category = "Money")
	bool RemoveMoney(int32 Amount);

	// Cash Register Configuration

	// If true, this component automatically binds to its owner's collision overlaps to consume money items.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money|Register")
	bool bActAsCashRegister;

	// Local multiplier applied only to money items consumed by this register
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money|Register", meta = (EditCondition = "bActAsCashRegister"))
	float LocalRegisterMultiplier;

private:
	UFUNCTION()
	void HandleGameStateMoneyChanged(int32 NewMoney);

	UFUNCTION()
	void OnOwnerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	class AGSGameState* GetGSGameState() const;
};
