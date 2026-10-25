#include "AstoniaNpc.h"
#include "AstoniaAppearance.h"
#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AAstoniaNpc::AAstoniaNpc()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	AutoPossessAI = EAutoPossessAI::Disabled;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	USkeletalMeshComponent* Body = GetMesh();
	if (Manny.Succeeded()) Body->SetSkeletalMesh(Manny.Object);
	if (Anim.Succeeded()) Body->SetAnimInstanceClass(Anim.Class);
	Body->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	Appearance = CreateDefaultSubobject<UAstoniaAppearanceComponent>(TEXT("Appearance"));

	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(RootComponent);
	NameTag->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
	NameTag->SetHorizontalAlignment(EHTA_Center);
	NameTag->SetWorldSize(22.f);
	NameTag->SetTextRenderColor(FColor::White);
	NameTag->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	QuestMarker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("QuestMarker"));
	QuestMarker->SetupAttachment(RootComponent);
	QuestMarker->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	QuestMarker->SetHorizontalAlignment(EHTA_Center);
	QuestMarker->SetWorldSize(60.f);
	QuestMarker->SetText(FText::FromString(TEXT("!")));
	QuestMarker->SetTextRenderColor(FColor(255, 210, 0));
	QuestMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	QuestMarker->SetVisibility(false);
}

void AAstoniaNpc::Setup(FName InTemplate, const FString& InDisplayName, EAstoniaNpcRole InRole, bool bFemale, FName Skin)
{
	Template = InTemplate;
	NpcRole = InRole;

	Appearance->Appearance.Skin = Skin;
	Appearance->Appearance.bFemale = bFemale;
	Appearance->Apply();

	NameTag->SetText(FText::FromString(InDisplayName));
	NameTag->SetTextRenderColor(NpcRole == EAstoniaNpcRole::Thief ? FColor(255, 90, 70) : FColor::White);
	QuestMarker->SetVisibility(NpcRole == EAstoniaNpcRole::QuestGiver);
}

void AAstoniaNpc::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep the name tag and marker turned to the camera.
	if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - NameTag->GetComponentLocation();
		const FRotator Facing(0.f, ToCam.Rotation().Yaw, 0.f);
		NameTag->SetWorldRotation(Facing);
		QuestMarker->SetWorldRotation(Facing);
	}
}
