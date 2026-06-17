// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GSPlayerInterface.h"
#include "MoverSimulationTypes.h"
#include "GSPawn.generated.h"

class UChaosCharacterMoverComponent;
class UCapsuleComponent;

UCLASS()
class PROJECTF_API AGSPawn : public APawn, public IGSPlayerInterface, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	AGSPawn();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UChaosCharacterMoverComponent> MoverComponent;

	FVector2D CachedMovementInput;
	bool bCachedJumpPressed;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void RequestMove_Implementation(const FVector2D& MovementVector) override;
	virtual void RequestJump_Implementation(bool bIsJumping) override;

	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;
};
