#include "Core/GSGameMode.h"
#include "Characters/GSPlayerState.h"

AGSGameMode::AGSGameMode()
{
	bUseSeamlessTravel = true;
	PlayerStateClass = AGSPlayerState::StaticClass();
}
