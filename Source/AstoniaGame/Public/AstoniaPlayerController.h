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

public:
	/** Closest usable IAstoniaInteractable in range of the pawn, or nullptr. */
	AActor* FindInteractable() const;

	/** Opens the character creator on the current pawn (also used when no character is saved yet). */
	UFUNCTION(Exec)
	void CreateCharacter();

	bool IsInCharacterCreator() const { return CreatorWidget.IsValid(); }

	UPROPERTY(BlueprintReadOnly, Category = "Astonia")
	FString CharacterName;

protected:
	virtual void SetupInputComponent() override;

	/** Loads the saved character onto the pawn, or opens the creator when there is none. */
	virtual void OnPossess(APawn* InPawn) override;

private:
	void FinishCharacter(const FString& Name);
	void RotatePreview(float Degrees);

	TSharedPtr<class SWidget> CreatorWidget;
	float SavedArmLength = 0.f;
	FVector SavedSocketOffset = FVector::ZeroVector;

	void BuildDefaultInput();
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted();
	void OnJumpCompleted();
	void OnInteract();

	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY()
	TObjectPtr<UInputAction> InteractAction;
};
