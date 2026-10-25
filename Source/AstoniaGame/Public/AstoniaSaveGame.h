#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AstoniaAppearance.h"
#include "AstoniaSaveGame.generated.h"

/** The player's character, saved by the character creator (slot "Player"). */
UCLASS()
class ASTONIAGAME_API UAstoniaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static const TCHAR* SlotName() { return TEXT("Player"); }

	/** Loads the saved character, or nullptr if none was created yet. */
	static UAstoniaSaveGame* Load();

	bool Save();

	UPROPERTY(SaveGame)
	FString CharacterName;

	UPROPERTY(SaveGame)
	FAstoniaAppearance Appearance;
};
