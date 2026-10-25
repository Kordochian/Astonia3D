#include "AstoniaSecretDoor.h"
#include "AstoniaCore.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAstoniaSecretDoor::AAstoniaSecretDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Block = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Block"));
	RootComponent = Block;
	Block->SetMobility(EComponentMobility::Movable);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Block->SetStaticMesh(Cube.Object);
	Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Astonia/Materials/M_PlaceholderSolid.M_PlaceholderSolid")));
}

void AAstoniaSecretDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLook();
}

void AAstoniaSecretDoor::BeginPlay()
{
	Super::BeginPlay();
	ApplyLook();
}

void AAstoniaSecretDoor::ApplyLook()
{
	// The cube is centred on the actor: place the actor at floor height + Height / 2.
	Block->SetRelativeScale3D(FVector(AstoniaTileSize / 100.f, AstoniaTileSize / 100.f, Height / 100.f));
	Block->SetCustomPrimitiveDataFloat(0, WallColor.R);
	Block->SetCustomPrimitiveDataFloat(1, WallColor.G);
	Block->SetCustomPrimitiveDataFloat(2, WallColor.B);
	if (UMaterialInterface* M = Material.LoadSynchronous()) Block->SetMaterial(0, M);
}

void AAstoniaSecretDoor::Interact(APawn* User)
{
	if (bOpen) return;
	bOpen = true;
	SetActorTickEnabled(true);
}

FText AAstoniaSecretDoor::GetInteractPrompt() const
{
	return FText::FromString(TEXT("Search the wall"));
}

void AAstoniaSecretDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Sink below the floor, then stop and stop colliding.
	const float Step = 150.f * DeltaSeconds;
	Sunk = FMath::Min(Sunk + Step, Height);
	AddActorWorldOffset(FVector(0.f, 0.f, -Step));
	if (Sunk >= Height)
	{
		Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Block->SetVisibility(false);
		SetActorTickEnabled(false);
	}
}
