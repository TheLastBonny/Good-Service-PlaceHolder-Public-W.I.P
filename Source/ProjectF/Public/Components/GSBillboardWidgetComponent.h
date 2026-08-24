#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GSBillboardWidgetComponent.generated.h"

/**
 * Custom Widget Component that automatically rotates towards the active player camera 
 * when operating in World Space (Billboard effect), and supports Screen Space seamlessly.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTF_API UGSBillboardWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UGSBillboardWidgetComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** If true and in World Space, the widget automatically rotates to face the player's camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard UI")
	bool bFaceCamera;

	/** Locks pitch (X axis rotation) so the widget stays upright without tilting back/forward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard UI")
	bool bLockPitch;

	/** Locks roll (Z axis rotation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard UI")
	bool bLockRoll;

	/** Maximum distance from camera to display the widget (0 = unlimited) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard UI", meta = (ClampMin = "0.0"))
	float MaxDrawDistance;
};
