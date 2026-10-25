#include "AstoniaLift.h"
#include "AstoniaCore.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AAstoniaLift::AAstoniaLift()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Platform top sits at the actor origin (floor height), 1.8 m square so it fits inside a 2 m shaft.
	Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
	Platform->SetupAttachment(Root);
	Platform->SetMobility(EComponentMobility::Movable);
	Platform->SetRelativeLocation(FVector(0.f, 0.f, -10.f));
	Platform->SetRelativeScale3D(FVector(AstoniaTileSize * 0.009f, AstoniaTileSize * 0.009f, 0.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Platform->SetStaticMesh(Cube.Object);
}

void AAstoniaLift::BeginPlay()
{
	Super::BeginPlay();
	Platform->SetRelativeLocation(FVector(0.f, 0.f, -10.f));
}

bool AAstoniaLift::IsPawnOnPlatform() const
{
	const UWorld* World = GetWorld();
	if (!World) return false;
	const FVector Top = Platform->GetComponentLocation() + FVector(0, 0, 10.f);
	const float Half = AstoniaTileSize * 0.45f;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
		if (!Pawn) continue;
		const FVector P = Pawn->GetActorLocation();
		if (FMath::Abs(P.X - Top.X) < Half && FMath::Abs(P.Y - Top.Y) < Half && P.Z > Top.Z && P.Z < Top.Z + 250.f) return true;
	}
	return false;
}

void AAstoniaLift::StartMove()
{
	if (bMoving) return;
	bMoving = true;
	StandTime = 0.f;
}

void AAstoniaLift::Interact(APawn* User)
{
	StartMove();
}

FVector AAstoniaLift::GetInteractLocation() const
{
	return Platform->GetComponentLocation() + FVector(0.f, 0.f, 90.f);
}

FText AAstoniaLift::GetInteractPrompt() const
{
	return Destination.IsEmpty()
		? FText::FromString(bAtBottom ? TEXT("Ride lift up") : TEXT("Ride lift down"))
		: FText::Format(FText::FromString(TEXT("Ride lift: {0}")), Destination);
}

void AAstoniaLift::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bMoving)
	{
		// Riders step off before the lift starts by itself again.
		if (IsPawnOnPlatform())
		{
			if (!bWaitForLeave)
			{
				StandTime += DeltaSeconds;
				if (StandTime >= StandDelay) StartMove();
			}
		}
		else
		{
			bWaitForLeave = false;
			StandTime = 0.f;
		}
		return;
	}

	const float TargetZ = (bAtBottom ? 0.f : BottomOffset) - 10.f;
	FVector Loc = Platform->GetRelativeLocation();
	const float Step = Speed * DeltaSeconds;
	if (FMath::Abs(TargetZ - Loc.Z) <= Step)
	{
		Loc.Z = TargetZ;
		bMoving = false;
		bAtBottom = !bAtBottom;
		bWaitForLeave = true;
	}
	else
	{
		Loc.Z += FMath::Sign(TargetZ - Loc.Z) * Step;
	}
	Platform->SetRelativeLocation(Loc);
}
