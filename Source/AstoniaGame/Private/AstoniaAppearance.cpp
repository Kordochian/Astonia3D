#include "AstoniaAppearance.h"
#include "AstoniaSkin.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const TCHAR* MalePath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	const TCHAR* FemalePath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple");

	void TintMaterials(UMeshComponent* Mesh, const FLinearColor& Color)
	{
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); Slot++)
		{
			if (UMaterialInstanceDynamic* M = Mesh->CreateDynamicMaterialInstance(Slot))
			{
				M->SetVectorParameterValue(TEXT("Tint"), Color);
			}
		}
	}
}

UAstoniaAppearanceComponent::UAstoniaAppearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PieceTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Characters/Appearance/DT_Pieces.DT_Pieces")));
	MaleMetaHumanBody = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_DefaultMale/MH_DefaultMale/Body/SKM_MH_DefaultMale_BodyMesh.SKM_MH_DefaultMale_BodyMesh")));
	MaleMetaHumanFace = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_DefaultMale/MH_DefaultMale/Face/SKM_MH_DefaultMale_FaceMesh.SKM_MH_DefaultMale_FaceMesh")));
	FemaleMetaHumanBody = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_DefaultFemale/MH_DefaultFemale/Body/SKM_MH_DefaultFemale_BodyMesh.SKM_MH_DefaultFemale_BodyMesh")));
	FemaleMetaHumanFace = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_DefaultFemale/MH_DefaultFemale/Face/SKM_MH_DefaultFemale_FaceMesh.SKM_MH_DefaultFemale_FaceMesh")));
}

void UAstoniaAppearanceComponent::ApplyMetaHuman(USkeletalMeshComponent* Driver)
{
	USkeletalMesh* BodyMesh = (Appearance.bFemale ? FemaleMetaHumanBody : MaleMetaHumanBody).LoadSynchronous();
	USkeletalMesh* FaceMesh = (Appearance.bFemale ? FemaleMetaHumanFace : MaleMetaHumanFace).LoadSynchronous();
	if (!BodyMesh)
	{
		Driver->SetVisibility(true, false);
		if (MetaBody) MetaBody->DestroyComponent();
		if (MetaFace) MetaFace->DestroyComponent();
		MetaBody = MetaFace = nullptr;
		return;
	}

	// The mannequin keeps playing the animation but is not drawn; the MetaHuman follows its bones.
	Driver->SetVisibility(false, false);
	Driver->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	auto Follow = [this, Driver](TObjectPtr<USkeletalMeshComponent>& Part, USkeletalMesh* Mesh)
	{
		if (!Mesh)
		{
			if (Part) Part->DestroyComponent();
			Part = nullptr;
			return;
		}
		if (!Part)
		{
			Part = NewObject<USkeletalMeshComponent>(GetOwner());
			Part->SetupAttachment(Driver);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Part->RegisterComponent();
		}
		Part->SetSkeletalMesh(Mesh);
		Part->SetLeaderPoseComponent(Driver);
	};
	Follow(MetaBody, BodyMesh);
	Follow(MetaFace, FaceMesh);
}

void UAstoniaAppearanceComponent::BeginPlay()
{
	Super::BeginPlay();
	Apply();
}

USkeletalMeshComponent* UAstoniaAppearanceComponent::GetBody() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character ? Character->GetMesh() : GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
}

void UAstoniaAppearanceComponent::Apply()
{
	USkeletalMeshComponent* Body = GetBody();
	if (!Body) return;

	if (!MaleBody) MaleBody = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(MalePath)).LoadSynchronous();
	if (!FemaleBody) FemaleBody = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(FemalePath)).LoadSynchronous();
	if (USkeletalMesh* Wanted = Appearance.bFemale ? FemaleBody : MaleBody)
	{
		if (Body->GetSkeletalMeshAsset() != Wanted) Body->SetSkeletalMesh(Wanted);
	}
	AstoniaSkin::Apply(Body, AstoniaSkin::Find(Appearance.Skin), Appearance.bFemale);
	ApplyMetaHuman(Body);

	for (const TPair<EAstoniaAppearanceSlot, TObjectPtr<UMeshComponent>>& Old : Spawned)
	{
		if (Old.Value) Old.Value->DestroyComponent();
	}
	Spawned.Reset();
	for (const TPair<EAstoniaAppearanceSlot, FName>& Piece : Appearance.Pieces)
	{
		ApplyPiece(Piece.Key);
	}
}

void UAstoniaAppearanceComponent::ApplyPiece(EAstoniaAppearanceSlot Slot)
{
	if (TObjectPtr<UMeshComponent>* Old = Spawned.Find(Slot))
	{
		if (*Old) (*Old)->DestroyComponent();
		Spawned.Remove(Slot);
	}

	const FName* PieceName = Appearance.Pieces.Find(Slot);
	const UDataTable* Table = PieceTable.LoadSynchronous();
	const FAstoniaAppearancePieceRow* Row = (PieceName && Table) ? Table->FindRow<FAstoniaAppearancePieceRow>(*PieceName, TEXT("Appearance"), false) : nullptr;
	USkeletalMeshComponent* Body = GetBody();
	if (!Row || !Body) return;

	UMeshComponent* Mesh = nullptr;
	if (USkeletalMesh* Skinned = Row->SkeletalMesh.LoadSynchronous())
	{
		USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(GetOwner());
		Part->SetSkeletalMesh(Skinned);
		Part->SetupAttachment(Body);
		Part->RegisterComponent();
		Part->SetLeaderPoseComponent(Body);
		Mesh = Part;
	}
	else if (UStaticMesh* Rigid = Row->StaticMesh.LoadSynchronous())
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(GetOwner());
		Part->SetStaticMesh(Rigid);
		Part->SetupAttachment(Body, Row->AttachTo);
		Part->SetRelativeTransform(Row->Offset);
		Part->RegisterComponent();
		Mesh = Part;
	}
	if (!Mesh) return;

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Row->bUseCharacterColor)
	{
		const FAstoniaSkinRow* Skin = AstoniaSkin::Find(Appearance.Skin);
		const bool bEyes = Slot == EAstoniaAppearanceSlot::Eyes;
		FLinearColor Color = bEyes ? Appearance.EyeColor : Appearance.HairColor;
		if (Color.A <= 0.f && Skin) Color = bEyes ? Skin->EyeColor : Skin->HairColor;
		TintMaterials(Mesh, Color);
	}
	else if (Row->Tint.A > 0.f)
	{
		TintMaterials(Mesh, Row->Tint);
	}
	Spawned.Add(Slot, Mesh);
}

void UAstoniaAppearanceComponent::SetPiece(EAstoniaAppearanceSlot Slot, FName Piece)
{
	Appearance.Pieces.Add(Slot, Piece);
	ApplyPiece(Slot);
}

void UAstoniaAppearanceComponent::ClearPiece(EAstoniaAppearanceSlot Slot)
{
	Appearance.Pieces.Remove(Slot);
	ApplyPiece(Slot);
}

void UAstoniaAppearanceComponent::SetSkin(FName Skin)
{
	Appearance.Skin = Skin;
	Apply();
}
