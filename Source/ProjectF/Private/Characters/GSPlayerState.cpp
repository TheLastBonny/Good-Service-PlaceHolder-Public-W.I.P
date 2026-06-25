#include "Characters/GSPlayerState.h"
#include "Net/UnrealNetwork.h"

AGSPlayerState::AGSPlayerState()
{
	bIsReady = false;
	bReplicates = true;
}

void AGSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGSPlayerState, bIsReady);
}

void AGSPlayerState::SetReady(bool bReady)
{
	if (HasAuthority())
	{
		bIsReady = bReady;
	}
}

void AGSPlayerState::OnRep_IsReady()
{
}
