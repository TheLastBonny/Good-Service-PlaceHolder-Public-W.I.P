#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GSGrabbableComponent.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FGSKinematicFlightParams
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsFlying = false;

	UPROPERTY()
	FVector LaunchVelocity = FVector::ZeroVector;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTF_API UGSGrabbableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSGrabbableComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	FName AttachmentSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	FTransform RelativeTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	TObjectPtr<UAnimMontage> GrabMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	TObjectPtr<UAnimMontage> ThrowMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	float LaunchForceMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	float ThrowSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	float ThrowArcHeight;

	// Height offset above the character head used as a fallback if the attachment socket is not found.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab")
	float FallbackAboveHeadHeight;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	float ProjectileBounciness;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	float ProjectileFriction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab")
	float ProjectileGravityScale;

	UFUNCTION(BlueprintCallable, Category = "Grab")
	void SetGrabbed(bool bInGrabbed);

	UFUNCTION(BlueprintPure, Category = "Grab")
	bool IsGrabbed() const;

	UFUNCTION()
	void OnRep_IsGrabbed();

	UPROPERTY(BlueprintReadWrite, Category = "Grab")
	bool bOriginalSimulatePhysics;

	UPROPERTY(BlueprintReadWrite, Category = "Grab")
	FName OriginalCollisionProfileName;

	UPROPERTY(BlueprintReadWrite, Category = "Grab")
	TEnumAsByte<ECollisionEnabled::Type> OriginalCollisionEnabled;

	// Projectile Flight Functions
	void LaunchKinematic(const FVector& InStartLoc, const FVector& InTargetLoc, float InArcHeight, float InDuration);

	UFUNCTION()
	void OnRep_KinematicFlightParams();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileComp;

protected:
	bool bIsFlying;

	UPROPERTY(ReplicatedUsing = OnRep_KinematicFlightParams)
	FGSKinematicFlightParams KinematicFlightParams;

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsGrabbed)
	bool bIsGrabbed;
};
