#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Characters/GSPlayerInterface.h"
#include "MoverSimulationTypes.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GSPawn.generated.h"

class UCharacterMoverComponent;
class UCapsuleComponent;
class UAbilitySystemComponent;
class UGSHealthAttributeSet;
class UGSMovementAttributeSet;
class UGSPatienceAttributeSet;
class UNavMoverComponent;
class UAudioComponent;
class UGSEmoteDefinition;

UCLASS()
class PROJECTF_API AGSPawn : public APawn, public IGSPlayerInterface, public IMoverInputProducerInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSPawn();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterMoverComponent> MoverComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNavMoverComponent> NavMoverComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UGSHealthAttributeSet> HealthSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UGSMovementAttributeSet> MovementSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UGSPatienceAttributeSet> PatienceSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TMap<FGameplayTag, FGameplayTag> AbilitySlotMap;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveEmoteAudioComponent;

	FVector2D CachedMovementInput;
	bool bCachedJumpPressed;
	bool bCachedJumpJustPressed;
	float TickLogTimer;

	void OnWalkSpeedChanged(const struct FOnAttributeChangeData& Data);
	void InitAbilityActorInfo();

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void RequestMove_Implementation(const FVector2D& MovementVector) override;
	virtual void RequestJump_Implementation(bool bIsJumping) override;

	virtual void RequestAbilityByTag_Implementation(const FGameplayTag& InputTag) override;
	virtual void RequestAbilityReleasedByTag_Implementation(const FGameplayTag& InputTag) override;

	UFUNCTION(BlueprintCallable, Category = "GAS")
	void AssignAbilityToSlot(FGameplayTag SlotTag, FGameplayTag AbilityTag);

	UFUNCTION(BlueprintPure, Category = "GAS")
	FGameplayTag GetAbilityTagForSlot(FGameplayTag SlotTag) const;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayEmoteSound(UGSEmoteDefinition* EmoteDef);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStopEmoteSound();

	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;
};
