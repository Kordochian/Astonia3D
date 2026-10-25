#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstoniaInteractable.h"
#include "AstoniaSecretDoor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/** A wall block that looks like the walls around it and sinks into the floor when used. */
UCLASS()
class ASTONIAGAME_API AAstoniaSecretDoor : public AActor, public IAstoniaInteractable
{
	GENERATED_BODY()

public:
	AAstoniaSecretDoor();

	virtual void Tick(float DeltaSeconds) override;

	virtual void Interact(APawn* User) override;
	virtual FText GetInteractPrompt() const override;
	virtual bool CanInteract() const override { return !bOpen; }
	virtual FVector GetInteractLocation() const override { return GetActorLocation(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	float Height = 300.f;

	/** Colour to blend in with the neighbouring walls (custom primitive data 0-2 of M_PlaceholderSolid). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor WallColor = FLinearColor(0.45f, 0.43f, 0.40f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	TSoftObjectPtr<UMaterialInterface> Material;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void ApplyLook();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Block;

	bool bOpen = false;
	float Sunk = 0.f;
};
