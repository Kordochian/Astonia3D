#include "AstoniaCharacter.h"
#include "AstoniaAppearance.h"
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
		}
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
		GetWorldTimerManager().SetTimer(Timer, [this]()
		{
			if (APlayerController* PC = Cast<APlayerController>(Controller)) PC->ConsoleCommand(TEXT("HighResShot 1280x720"));
		}, Delay, false);
	}
#endif
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
