#include "Core/GSGameMode.h"
#include "Characters/GSPlayerState.h"
#include "Core/GSGameState.h"

AGSGameMode::AGSGameMode()
{
	bUseSeamlessTravel = true;
	PlayerStateClass = AGSPlayerState::StaticClass();
	GameStateClass = AGSGameState::StaticClass();
}

