#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstoniaCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * Player character with a free third-person orbit camera. The body is Epic's UE5 mannequin until
 * Astonia's own character art exists; stats, combat and equipment get added in later phases.
 */
UCLASS()
class ASTONIAGAME_API AAstoniaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAstoniaCharacter();

	void DoMove(float Right, float Forward);
	void DoLook(float Yaw, float Pitch);

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

protected:
	/**
	 * Development only: -AstoniaGoto=X,Y,Z,Yaw puts the player there on start, -AstoniaShot takes a
	 * 1280x720 screenshot three seconds later (Saved/Screenshots). Used to check builds headlessly.
	 */
	virtual void BeginPlay() override;

public:
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Look of the player's mannequin (row of DT_Skins). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FName Skin = TEXT("Adventurer");

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;
};
