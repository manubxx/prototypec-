#include "cybmineGameMode.h"
#include "cybmineCharacter.h"
#include "cybmineHUD.h"

AcybmineGameMode::AcybmineGameMode()
{
	DefaultPawnClass = AcybmineCharacter::StaticClass();
	HUDClass = AcybmineHUD::StaticClass();
}