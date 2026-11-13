#include "cybmineGameMode.h"
#include "cybmineCharacter.h"
#include "cybmineHUD.h"
#include "cybminePlayerState.h"

AcybmineGameMode::AcybmineGameMode()
{

	PlayerStateClass = AcybminePlayerState::StaticClass();
	DefaultPawnClass = AcybmineCharacter::StaticClass();
	HUDClass = AcybmineHUD::StaticClass();
}