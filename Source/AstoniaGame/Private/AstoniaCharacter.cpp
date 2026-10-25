#include "AstoniaCharacter.h"
#include "AstoniaAppearance.h"
#include "AstoniaCore.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AAstoniaCharacter::AAstoniaCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 500.f, 0.f);
	Move->JumpZVelocity = 500.f;
	Move->AirControl = 0.35f;
	Move->MaxWalkSpeed = 500.f;
	Move->MinAnalogWalkSpeed = 20.f;
	Move->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Body: Epic's UE5 mannequin (free with the engine, from the Third Person template resources) with
	// its unarmed locomotion animation blueprint. Mesh feet at the capsule bottom, facing +X. The
	// appearance component swaps in the female body and the skin material.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Body(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (Body.Succeeded()) BodyMesh->SetSkeletalMesh(Body.Object);
	if (Anim.Succeeded()) BodyMesh->SetAnimInstanceClass(Anim.Class);
	BodyMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	Appearance = CreateDefaultSubobject<UAstoniaAppearanceComponent>(TEXT("Appearance"));
}

void AAstoniaCharacter::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	FString Goto;
	if (FParse::Value(FCommandLine::Get(), TEXT("AstoniaGoto="), Goto, /*bShouldStopOnSeparator=*/false))
	{
		TArray<FString> Parts;
		Goto.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() >= 3)
		{
			SetActorLocation(FVector(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2])));
			if (Parts.Num() >= 4)
			{
				const float Pitch = Parts.Num() >= 5 ? FCString::Atof(*Parts[4]) : -10.f;
				const FRotator Facing(Pitch, FCString::Atof(*Parts[3]), 0.f);
				SetActorRotation(FRotator(0.f, Facing.Yaw, 0.f));
				if (Controller) Controller->SetControlRotation(Facing);
			}
			// -AstoniaFly: hover at the spot (aerial screenshots).
			if (FParse::Param(FCommandLine::Get(), TEXT("AstoniaFly"))) GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		}
	}
	FString RouteArg;
	if (FParse::Value(FCommandLine::Get(), TEXT("AstoniaRoute="), RouteArg, false))
	{
		TArray<FString> Points;
		RouteArg.ParseIntoArray(Points, TEXT(";"));
		for (const FString& Point : Points)
		{
			FString X, Y;
			if (Point.Split(TEXT(","), &X, &Y)) Route.Add(FVector2D(FCString::Atof(*X), FCString::Atof(*Y)));
		}
		RouteTime = GetWorld()->GetTimeSeconds() + 5.f;  // let the world settle first
	}
	FString SpawnBP;
	if (FParse::Value(FCommandLine::Get(), TEXT("AstoniaSpawn="), SpawnBP, false))
	{
		// path,x,y,z,yaw: spawns a Blueprint actor, e.g. an assembled MetaHuman, to compare against.
		TArray<FString> P;
		SpawnBP.ParseIntoArray(P, TEXT(","));
		if (P.Num() >= 5)
		{
			if (UClass* Class = LoadClass<AActor>(nullptr, *P[0]))
			{
				GetWorld()->SpawnActor<AActor>(Class, FVector(FCString::Atof(*P[1]), FCString::Atof(*P[2]), FCString::Atof(*P[3])),
					FRotator(0.f, FCString::Atof(*P[4]), 0.f));
			}
		}
	}
	float Arm = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("AstoniaArm="), Arm))
	{
		CameraBoom->TargetArmLength = Arm;
		CameraBoom->SocketOffset = FVector(0.f, 0.f, Arm > 0.f ? 60.f : 70.f);
		if (Arm <= 0.f)
		{
			// Eye view: don't draw our own head and body from the inside.
			TArray<USceneComponent*> Parts;
			GetMesh()->GetChildrenComponents(true, Parts);
			for (USceneComponent* Part : Parts)
			{
				if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Part)) Prim->SetOwnerNoSee(true);
			}
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("AstoniaShot")))
	{
		float Delay = 3.f;
		FParse::Value(FCommandLine::Get(), TEXT("AstoniaShotDelay="), Delay);
		FTimerHandle Timer;
		// -AstoniaShotUI includes menus and the HUD (plain "shot showui" at window size).
		const bool bUI = FParse::Param(FCommandLine::Get(), TEXT("AstoniaShotUI"));
		GetWorldTimerManager().SetTimer(Timer, [this, bUI]()
		{
			if (APlayerController* PC = Cast<APlayerController>(Controller)) PC->ConsoleCommand(bUI ? TEXT("shot showui") : TEXT("HighResShot 1280x720"));
		}, Delay, false);
	}
#endif
}

void AAstoniaCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Route.IsEmpty() && GetWorld()->GetTimeSeconds() > RouteTime) StepRoute();
}

void AAstoniaCharacter::StepRoute()
{
	if (!Route.IsValidIndex(RouteIndex))
	{
		UE_LOG(LogAstoniaCore, Display, TEXT("AstoniaRoute: finished at %s"), *GetActorLocation().ToString());
		Route.Reset();
		return;
	}
	const FVector Here = GetActorLocation();
	const FVector2D Delta = Route[RouteIndex] - FVector2D(Here);
	const float Dist = Delta.Size();
	if (Dist < 60.f)
	{
		UE_LOG(LogAstoniaCore, Display, TEXT("AstoniaRoute: reached %d at %s"), RouteIndex, *Here.ToString());
		RouteIndex++;
		RouteStuck = 0.f;
		RouteBest = MAX_flt;
		return;
	}
	// Stuck = no 20 cm of progress towards the waypoint for 3 s of game time (timer catch-up calls in
	// one frame add nothing).
	const float Now = GetWorld()->GetTimeSeconds();
	const float Step = Now - RouteTime;
	RouteTime = Now;
	if (Dist < RouteBest - 20.f)
	{
		RouteBest = Dist;
		RouteStuck = 0.f;
	}
	else if ((RouteStuck += Step) > 3.f)
	{
		UE_LOG(LogAstoniaCore, Warning, TEXT("AstoniaRoute: stuck before %d at %s (mode %d, input ignored %d, controller %s)"), RouteIndex,
			*Here.ToString(), int32(GetCharacterMovement()->MovementMode), IsMoveInputIgnored(), *GetNameSafe(Controller));
		Route.Reset();
		return;
	}
	AddMovementInput(FVector(Delta.X, Delta.Y, 0.f).GetSafeNormal());
}

void AAstoniaCharacter::DoMove(float Right, float Forward)
{
	if (!Controller) return;

	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Forward);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Right);
}

void AAstoniaCharacter::DoLook(float Yaw, float Pitch)
{
	if (!Controller) return;

	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}
