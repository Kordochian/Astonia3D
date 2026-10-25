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

	// Until each NPC gets its own look, vary hair, hair colour and skin shade by template name.
	static const FName MaleHair[] = {"Hair_S_Casual", "Hair_S_Messy", "Hair_S_SlickBack", "Hair_S_BrushCut", "Hair_M_Layered", "Hair_S_RecedeMessy"};
	static const FName FemaleHair[] = {"Hair_M_BobStraight", "Hair_L_Straight", "Hair_S_LowPonytail", "Hair_S_Updo", "Hair_M_Layered", "Hair_S_UpdoBraids"};
	static const FName Beards[] = {NAME_None, "Beard_S_Stubble", "Beard_M_Stubble", NAME_None, "Beard_S_Full", "Goatee_M_Pointed"};
	const uint32 Hash = GetTypeHash(InTemplate.ToString());
	FAstoniaAppearance& A = Appearance->Appearance;
	A.Skin = Skin;
	A.bFemale = bFemale;
	A.HairStyle = bFemale ? FemaleHair[Hash % 6] : MaleHair[Hash % 6];
	A.Beard = bFemale ? NAME_None : Beards[(Hash / 7) % 6];
	A.HairMelanin = 0.15f + 0.85f * float((Hash / 13) % 8) / 7.f;
	A.HairRedness = 0.1f + 0.5f * float((Hash / 17) % 4) / 3.f;
	A.SkinShade = 0.5f + 0.75f * float((Hash / 19) % 6) / 5.f;
	Appearance->Apply();

	NameTag->SetText(FText::FromString(InDisplayName));
	NameTag->SetTextRenderColor(NpcRole == EAstoniaNpcRole::Thief ? FColor(255, 90, 70) : FColor::White);
	QuestMarker->SetVisibility(NpcRole == EAstoniaNpcRole::QuestGiver);
}

void AAstoniaNpc::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Cam) return;

	// Level of detail: unseen or far NPCs stop animating (the hidden mannequin drives the MetaHuman, so
	// it would otherwise always tick), far ones drop hair and name tag.
	const float Dist = FVector::Dist(GetActorLocation(), Cam->GetCameraLocation());
	const bool bSeen = Appearance->WasBodyRecentlyRendered(0.3f);
	const bool bAnimate = bSeen && Dist < AnimateDistance;
	if (GetMesh()->IsComponentTickEnabled() != bAnimate) GetMesh()->SetComponentTickEnabled(bAnimate);
	Appearance->SetGroomsVisible(Dist < GroomDistance);
	const bool bTag = bSeen && Dist < TagDistance;
	if (NameTag->IsVisible() != bTag) NameTag->SetVisibility(bTag);
	if (NpcRole == EAstoniaNpcRole::QuestGiver && QuestMarker->IsVisible() != bSeen) QuestMarker->SetVisibility(bSeen);
	if (!bTag) return;

	// Keep the name tag and marker turned to the camera.
	const FVector ToCam = Cam->GetCameraLocation() - NameTag->GetComponentLocation();
	const FRotator Facing(0.f, ToCam.Rotation().Yaw, 0.f);
	NameTag->SetWorldRotation(Facing);
	QuestMarker->SetWorldRotation(Facing);
}
