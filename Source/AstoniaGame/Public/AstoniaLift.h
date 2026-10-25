#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstoniaInteractable.h"
#include "AstoniaLift.generated.h"

class UStaticMeshComponent;

/**
 * A platform that rides between two heights. Replaces the original teleport holes and ladders, so
 * basements and caverns are real places under their entrances. Starts when the player stands on
 * it for a moment or presses E.
 */
UCLASS()
class ASTONIAGAME_API AAstoniaLift : public AActor, public IAstoniaInteractable
{
	GENERATED_BODY()

public:
	AAstoniaLift();

	virtual void Tick(float DeltaSeconds) override;

	virtual void Interact(APawn* User) override;
	virtual FText GetInteractPrompt() const override;
	virtual bool CanInteract() const override { return !bMoving; }
	virtual FVector GetInteractLocation() const override;

	/** Height of the lower stop relative to the actor (negative = below). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	float BottomOffset = -600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	float Speed = 250.f;

	/** Seconds the player has to stand on the platform before it starts by itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	float StandDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FText Destination;

protected:
	virtual void BeginPlay() override;

private:
	void StartMove();
	bool IsPawnOnPlatform() const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Platform;

	bool bAtBottom = false;
	bool bMoving = false;
	bool bWaitForLeave = false;
	float StandTime = 0.f;
};
