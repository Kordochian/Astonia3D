#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstoniaPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Builds the default key bindings in code (WASD/mouse, gamepad sticks, Space) so the project
 * runs without any input assets. Swap for asset-based contexts once there is a settings menu.
 */
UCLASS()
class ASTONIAGAME_API AAstoniaPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

private:
	void BuildDefaultInput();
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted();
	void OnJumpCompleted();

	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;
};
