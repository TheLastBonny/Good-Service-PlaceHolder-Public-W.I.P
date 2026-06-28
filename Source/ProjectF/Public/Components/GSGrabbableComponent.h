#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSGrabbableComponent.generated.h"

class UAnimMontage;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTF_API UGSGrabbableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSGrabbableComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	UFUNCTION(BlueprintCallable, Category = "Grab")
	void SetGrabbed(bool bInGrabbed);

	UFUNCTION(BlueprintPure, Category = "Grab")
	bool IsGrabbed() const;

private:
	UPROPERTY(Replicated)
	bool bIsGrabbed;
};
