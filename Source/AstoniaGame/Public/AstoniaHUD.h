#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AstoniaHUD.generated.h"

/** Placeholder HUD. Always shows the attribution the Astonia source license asks for. */
UCLASS()
class ASTONIAGAME_API AAstoniaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
