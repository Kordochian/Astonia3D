#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstoniaCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;

/**
 * Player character with a free third-person orbit camera. Until real art exists the body is an
 * engine cylinder; stats, combat and equipment get added as components in later phases.
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

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;
};
