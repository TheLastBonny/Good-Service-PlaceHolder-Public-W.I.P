#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AbilitySystemInterface.h"
#include "GSGameState.generated.h"

UENUM(BlueprintType)
enum class EGSGamePhase : uint8
{
	WaitingToStart,
	RoundInProgress,
	RoundOver
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChangedSignature, int32, NewMoney);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyMultiplierChangedSignature, float, NewMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChangedSignature, EGSGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemainingTimeChangedSignature, float, NewTime);

class UAbilitySystemComponent;
class UGSMoneyAttributeSet;
class AGSNPCManager;

UCLASS()
class PROJECTF_API AGSGameState : public AGameStateBase, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSGameState();

	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// Delegates for UI / gameplay bindings
	UPROPERTY(BlueprintAssignable, Category = "Game|Money")
	FOnMoneyChangedSignature OnMoneyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game|Money")
	FOnMoneyMultiplierChangedSignature OnMoneyMultiplierChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game|Cycle")
	FOnGamePhaseChangedSignature OnGamePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game|Cycle")
	FOnRemainingTimeChangedSignature OnRemainingTimeChanged;

	// Money Getters & Helpers
	UFUNCTION(BlueprintPure, Category = "Game|Money")
	int32 GetMoney() const;

	UFUNCTION(BlueprintPure, Category = "Game|Money")
	float GetMoneyMultiplier() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Money")
	void AddMoneyDirectly(float BaseAmount);

	// Game Cycle Getters & Setters
	UFUNCTION(BlueprintPure, Category = "Game|Cycle")
	EGSGamePhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Cycle")
	void SetGamePhase(EGSGamePhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Game|Cycle")
	float GetRemainingTime() const { return RemainingTime; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Cycle")
	void SetRemainingTime(float NewTime);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Cycle")
	void StartRoundTimer();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Cycle")
	void StopRoundTimer();

	// NPC Manager Registration
	UFUNCTION(BlueprintCallable, Category = "Game|NPCs")
	void RegisterNPCManager(AGSNPCManager* Manager);

	UFUNCTION(BlueprintPure, Category = "Game|NPCs")
	AGSNPCManager* GetNPCManager() const { return NPCManager; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UGSMoneyAttributeSet> MoneyAttributeSet;

	// The current round phase (replicated)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, VisibleAnywhere, BlueprintReadOnly, Category = "Game|Cycle")
	EGSGamePhase CurrentPhase;

	// Remaining round time in seconds (replicated)
	UPROPERTY(ReplicatedUsing = OnRep_RemainingTime, VisibleAnywhere, BlueprintReadOnly, Category = "Game|Cycle")
	float RemainingTime;

	// Reference to the NPC spawner manager
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game|NPCs")
	TObjectPtr<AGSNPCManager> NPCManager;

	UFUNCTION()
	void OnRep_CurrentPhase(EGSGamePhase OldPhase);

	UFUNCTION()
	void OnRep_RemainingTime(float OldTime);

private:
	FTimerHandle RoundTimerHandle;

	void DecrementRoundTime();
};
