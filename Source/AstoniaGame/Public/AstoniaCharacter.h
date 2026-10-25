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
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;
};
