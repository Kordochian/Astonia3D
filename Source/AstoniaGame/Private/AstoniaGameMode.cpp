#include "AstoniaGameMode.h"
#include "AstoniaCharacter.h"
#include "AstoniaHUD.h"
#include "AstoniaPlayerController.h"

AAstoniaGameMode::AAstoniaGameMode()
{
	DefaultPawnClass = AAstoniaCharacter::StaticClass();
	PlayerControllerClass = AAstoniaPlayerController::StaticClass();
	HUDClass = AAstoniaHUD::StaticClass();
}
