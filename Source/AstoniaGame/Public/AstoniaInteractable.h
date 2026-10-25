#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AstoniaInteractable.generated.h"

UINTERFACE(MinimalAPI)
class UAstoniaInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Something the player can use with the interact key (E): lifts, secret doors, later doors and chests. */
class ASTONIAGAME_API IAstoniaInteractable
{
	GENERATED_BODY()

public:
	virtual void Interact(APawn* User) = 0;

	/** Short prompt for the HUD, e.g. "Ride lift". */
	virtual FText GetInteractPrompt() const = 0;

	/** False while busy (a moving lift, an open door). */
	virtual bool CanInteract() const { return true; }

	/** Where the user has to be near (e.g. the lift platform, wherever it is now). */
	virtual FVector GetInteractLocation() const = 0;

	/** Range check from the user's location. */
	virtual float GetInteractRange() const { return 250.f; }
};
