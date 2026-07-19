#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/PlayerCameraManager.h"
#include "GSCameraTriggerVolume.generated.h"

class UBoxComponent;
class UCameraComponent;

UCLASS()
class PROJECTF_API AGSCameraTriggerVolume : public AActor
{
	GENERATED_BODY()

public:
	AGSCameraTriggerVolume();

	AActor* GetCustomCameraTarget() const { return CustomCameraTarget.IsValid() ? CustomCameraTarget.Get() : nullptr; }
	float GetBlendTime() const { return BlendTime; }
	EViewTargetBlendFunction GetBlendFunction() const { return BlendFunction; }
	float GetBlendExp() const { return BlendExp; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BlendTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BlendExp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TWeakObjectPtr<AActor> CustomCameraTarget;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
