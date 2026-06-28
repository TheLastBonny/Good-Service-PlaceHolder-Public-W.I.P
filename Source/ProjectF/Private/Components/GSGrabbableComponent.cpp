#include "Components/GSGrabbableComponent.h"
#include "Net/UnrealNetwork.h"

UGSGrabbableComponent::UGSGrabbableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	AttachmentSocketName = TEXT("HoldingSocket");
	RelativeTransform = FTransform::Identity;
	LaunchForceMultiplier = 1.0f;
	bIsGrabbed = false;
}

void UGSGrabbableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGSGrabbableComponent, bIsGrabbed);
}

void UGSGrabbableComponent::SetGrabbed(bool bInGrabbed)
{
	bIsGrabbed = bInGrabbed;
}

bool UGSGrabbableComponent::IsGrabbed() const
{
	return bIsGrabbed;
}
