#include "AstoniaSaveGame.h"
#include "Kismet/GameplayStatics.h"

UAstoniaSaveGame* UAstoniaSaveGame::Load()
{
	return UGameplayStatics::DoesSaveGameExist(SlotName(), 0)
		? Cast<UAstoniaSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0))
		: nullptr;
}

bool UAstoniaSaveGame::Save()
{
	return UGameplayStatics::SaveGameToSlot(this, SlotName(), 0);
}
