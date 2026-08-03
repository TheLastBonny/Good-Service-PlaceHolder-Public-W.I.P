#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSMoneyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComponentOnMoneyChangedSignature, int32, NewMoney);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComponentOnMoneyDepositedSignature, AActor*, MoneyActor, int32, FinalAddedValue);

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

	UPROPERTY(BlueprintAssignable, Category = "Money")
	FComponentOnMoneyChangedSignature OnMoneyChanged;


	UPROPERTY(BlueprintAssignable, Category = "Money|Register")
	FComponentOnMoneyDepositedSignature OnMoneyDeposited;


	UFUNCTION(BlueprintPure, Category = "Money")
	int32 GetCurrentMoney() const;


	UFUNCTION(BlueprintPure, Category = "Money")
	float GetGlobalMoneyMultiplier() const;


	UFUNCTION(BlueprintCallable, Category = "Money")
	void AddMoney(int32 Amount);


	UFUNCTION(BlueprintCallable, Category = "Money")
	bool RemoveMoney(int32 Amount);




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money|Register")
	bool bActAsCashRegister;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money|Register", meta = (EditCondition = "bActAsCashRegister"))
	float LocalRegisterMultiplier;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money|Register", meta = (EditCondition = "bActAsCashRegister"))
	TSubclassOf<class UGameplayEffect> DefaultMoneyEffectClass;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money|Debug")
	bool bShowDebugLogs = false;

	UFUNCTION(CallInEditor, Category = "Money|Debug")
	void ToggleDebugLogs() { bShowDebugLogs = !bShowDebugLogs; }

private:
	UFUNCTION()
	void HandleGameStateMoneyChanged(int32 NewMoney);

	UFUNCTION()
	void OnOwnerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	class AGSGameState* GetGSGameState() const;
};
