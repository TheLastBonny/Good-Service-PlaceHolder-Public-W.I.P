#include "Components/GSBillboardWidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UGSBillboardWidgetComponent::UGSBillboardWidgetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;

	bFaceCamera = true;
	bLockPitch = true;
	bLockRoll = true;
	MaxDrawDistance = 0.0f;

	SetWidgetSpace(EWidgetSpace::World);
	SetTwoSided(true);
	SetDrawSize(FVector2D(250.0f, 120.0f));
	SetPivot(FVector2D(0.5f, 0.5f));
}

void UGSBillboardWidgetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsVisible() || !GetWorld()) return;

	if (GetWidgetSpace() == EWidgetSpace::World && bFaceCamera)
	{
		FVector CameraLocation = FVector::ZeroVector;
		bool bHasCamera = false;

		APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
		if (CameraManager)
		{
			CameraLocation = CameraManager->GetCameraLocation();
			bHasCamera = true;
		}
		else if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
				bHasCamera = true;
			}
		}

		if (bHasCamera)
		{
			FVector ComponentLocation = GetComponentLocation();

			if (MaxDrawDistance > 0.0f)
			{
				float DistSq = FVector::DistSquared(ComponentLocation, CameraLocation);
				if (DistSq > (MaxDrawDistance * MaxDrawDistance))
				{
					return;
				}
			}

			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(ComponentLocation, CameraLocation);

			if (bLockPitch)
			{
				LookAtRotation.Pitch = 0.0f;
			}
			if (bLockRoll)
			{
				LookAtRotation.Roll = 0.0f;
			}

			SetWorldRotation(LookAtRotation);
		}
	}
}
