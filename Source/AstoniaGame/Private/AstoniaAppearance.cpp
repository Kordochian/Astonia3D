#include "AstoniaAppearance.h"
#include "AstoniaSkin.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"
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

namespace
{
	const TCHAR* GroomRoot = TEXT("/MetaHumanCharacter/Optional/Grooms/GroomAssets");
	const TCHAR* HairMaterialPath = TEXT("/MetaHumanCharacter/Lookdev_UHM/Hair/Materials/MI_hair_unified.MI_hair_unified");

	const TCHAR* KindFolder(EAstoniaGroomKind Kind)
	{
		switch (Kind)
		{
		case EAstoniaGroomKind::Hair: return TEXT("Hair");
		case EAstoniaGroomKind::Eyebrows: return TEXT("Eyebrows");
		case EAstoniaGroomKind::Beards: return TEXT("Beards");
		default: return TEXT("Mustaches");
		}
	}

	/** A fresh dynamic instance of a slot's assembled material (not of an earlier dynamic copy). */
	UMaterialInstanceDynamic* FreshInstance(UMeshComponent* Mesh, int32 Slot)
	{
		UMaterialInterface* Current = Mesh->GetMaterial(Slot);
		if (UMaterialInstanceDynamic* Old = Cast<UMaterialInstanceDynamic>(Current)) Current = Old->Parent;
		if (!Current) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Current, Mesh);
		Mesh->SetMaterial(Slot, M);
		return M;
	}

}

const TArray<FName>& AstoniaGrooms::Styles(EAstoniaGroomKind Kind)
{
	// The styles that ship with MetaHuman Creator Core Data (UE 5.8).
	static const TArray<FName> Hair = {
		"Hair_L_AfroCurly", "Hair_L_MessyClumps", "Hair_L_Straight", "Hair_L_StraightBangs", "Hair_M_BobBangs",
		"Hair_M_BobCurly", "Hair_M_BobMessy", "Hair_M_BobSlick", "Hair_M_BobStraight", "Hair_M_FauxMohawk",
		"Hair_M_Layered", "Hair_M_Mohawk", "Hair_M_SideSweptFringe", "Hair_M_TwistedBraids", "Hair_S_360Waves",
		"Hair_S_AfroFade", "Hair_S_BaldingStubble", "Hair_S_BobLayered", "Hair_S_BrushCut", "Hair_S_BuzzCut",
		"Hair_S_Casual", "Hair_S_Clean", "Hair_S_Coil", "Hair_S_CoilBuzzCut", "Hair_S_Cornrows", "Hair_S_CurlyFade",
		"Hair_S_HairLoss", "Hair_S_LowPonytail", "Hair_S_Messy", "Hair_S_Pixie", "Hair_S_PulledBack",
		"Hair_S_RecedeMessy", "Hair_S_SideSweptFringe", "Hair_S_SlickBack", "Hair_S_SweptUp", "Hair_S_Updo",
		"Hair_S_UpdoBraids", "Hair_S_UpdoBuns"};
	static const TArray<FName> Eyebrows = {
		"Eyebrows_L_Shaded", "Eyebrows_M_Dense", "Eyebrows_M_Fine", "Eyebrows_M_FlatThick", "Eyebrows_M_Full",
		"Eyebrows_M_Messy", "Eyebrows_M_Natural", "Eyebrows_M_Scraggly", "Eyebrows_M_Shaded", "Eyebrows_M_SlightArch",
		"Eyebrows_M_Slit", "Eyebrows_M_Swept", "Eyebrows_M_Thick", "Eyebrows_M_Thin", "Eyebrows_M_Unruly",
		"Eyebrows_M_Wide", "Eyebrows_S_FlatThin", "Eyebrows_S_Shaded"};
	static const TArray<FName> Beards = {
		"Beard_L_Full", "Beard_L_Messy", "Beard_M_Curly", "Beard_M_MuttonChops", "Beard_M_Stubble", "Beard_S_Curly",
		"Beard_S_Full", "Beard_S_PencilThin", "Beard_S_Stubble", "Beard_S_Uneven", "Goatee_L_Scraggly", "Goatee_L_Wavy",
		"Goatee_M_Pointed", "Goatee_M_Soulpatch", "Goatee_S_ChinStrap", "Goatee_S_SoulpatchStrip"};
	static const TArray<FName> Mustaches = {
		"Mustache_L_Full", "Mustache_L_Handlebar", "Mustache_L_Messy", "Mustache_L_Scraggly", "Mustache_L_Stubble",
		"Mustache_L_Wavy", "Mustache_M_Curly", "Mustache_M_Gunslinger", "Mustache_M_Stubble", "Mustache_S_ChinStrap",
		"Mustache_S_Curly", "Mustache_S_Full", "Mustache_S_Horseshoe", "Mustache_S_PencilThin", "Mustache_S_Stubble",
		"Mustache_S_Uneven"};
	switch (Kind)
	{
	case EAstoniaGroomKind::Hair: return Hair;
	case EAstoniaGroomKind::Eyebrows: return Eyebrows;
	case EAstoniaGroomKind::Beards: return Beards;
	default: return Mustaches;
	}
}

FText AstoniaGrooms::DisplayName(FName Style)
{
	if (Style.IsNone()) return FText::FromString(TEXT("None"));
	TArray<FString> Parts;
	Style.ToString().ParseIntoArray(Parts, TEXT("_"));
	if (Parts.Num() < 3) return FText::FromName(Style);
	// CamelCase words get spaces; the size letter becomes (short / medium / long).
	FString Words;
	for (int32 i = 2; i < Parts.Num(); i++)
	{
		for (int32 c = 0; c < Parts[i].Len(); c++)
		{
			if (c > 0 && FChar::IsUpper(Parts[i][c]) && !FChar::IsUpper(Parts[i][c - 1])) Words.AppendChar(' ');
			Words.AppendChar(Parts[i][c]);
		}
		if (i + 1 < Parts.Num()) Words.AppendChar(' ');
	}
	const TCHAR* Size = Parts[1] == TEXT("S") ? TEXT("short") : Parts[1] == TEXT("M") ? TEXT("medium") : TEXT("long");
	return FText::FromString(FString::Printf(TEXT("%s (%s)"), *Words, Size));
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
	ApplyMetaHumanLook();
}

bool UAstoniaAppearanceComponent::WasBodyRecentlyRendered(float Seconds) const
{
	return MetaBody ? MetaBody->WasRecentlyRendered(Seconds) : true;
}

void UAstoniaAppearanceComponent::SetGroomsVisible(bool bVisible)
{
	for (const TPair<EAstoniaGroomKind, TObjectPtr<UGroomComponent>>& Groom : Grooms)
	{
		if (Groom.Value && Groom.Value->IsVisible() != bVisible) Groom.Value->SetVisibility(bVisible);
	}
}

void UAstoniaAppearanceComponent::RefreshLook()
{
	ApplyMetaHumanLook();
}

void UAstoniaAppearanceComponent::ApplyMetaHumanLook()
{
	if (!MetaBody) return;

	// Skin: the baked skin materials take a brightness and saturation after the bake.
	for (UMeshComponent* Mesh : {static_cast<UMeshComponent*>(MetaBody.Get()), static_cast<UMeshComponent*>(MetaFace.Get())})
	{
		if (!Mesh) continue;
		const TArray<FName> Slots = Mesh->GetMaterialSlotNames();
		for (int32 Slot = 0; Slot < Slots.Num(); Slot++)
		{
			const FString SlotName = Slots[Slot].ToString();
			const bool bSkin = SlotName.StartsWith(TEXT("head_")) || SlotName.StartsWith(TEXT("body_"));
			const bool bEye = SlotName.StartsWith(TEXT("eyeLeft")) || SlotName.StartsWith(TEXT("eyeRight"));
			if (!bSkin && !(bEye && Appearance.EyeColor.A > 0.f)) continue;
			UMaterialInstanceDynamic* M = FreshInstance(Mesh, Slot);
			if (!M) continue;
			if (bSkin)
			{
				float Value = 1.f, Saturation = 1.f;
				M->GetScalarParameterValue(TEXT("Basecolor Global Value Post-Bake"), Value);
				M->GetScalarParameterValue(TEXT("Basecolor Global Saturation Post-Bake"), Saturation);
				M->SetScalarParameterValue(TEXT("Basecolor Global Value Post-Bake"), Value * Appearance.SkinShade);
				M->SetScalarParameterValue(TEXT("Basecolor Global Saturation Post-Bake"), Saturation * Appearance.SkinSaturation);
			}
			else
			{
				M->SetVectorParameterValue(TEXT("Iris Color Multiply"), Appearance.EyeColor);
			}
		}
	}

	ApplyGroom(EAstoniaGroomKind::Hair, Appearance.HairStyle);
	ApplyGroom(EAstoniaGroomKind::Eyebrows, Appearance.Eyebrows);
	ApplyGroom(EAstoniaGroomKind::Beards, Appearance.Beard);
	ApplyGroom(EAstoniaGroomKind::Mustaches, Appearance.Mustache);
}

void UAstoniaAppearanceComponent::ApplyGroom(EAstoniaGroomKind Kind, FName Style)
{
	TObjectPtr<UGroomComponent>& Groom = Grooms.FindOrAdd(Kind);
	USkeletalMeshComponent* Head = MetaFace ? MetaFace.Get() : MetaBody.Get();
	UGroomAsset* Asset = nullptr;
	const FString Name = Style.ToString();
	if (!Style.IsNone() && Head)
	{
		Asset = LoadObject<UGroomAsset>(nullptr, *FString::Printf(TEXT("%s/%s/%s/%s.%s"), GroomRoot, KindFolder(Kind), *Name, *Name, *Name));
	}
	if (!Asset)
	{
		if (Groom) Groom->DestroyComponent();
		Groom = nullptr;
		return;
	}
	if (!Groom)
	{
		Groom = NewObject<UGroomComponent>(GetOwner());
		Groom->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Groom->RegisterComponent();
	}

	// Bindings (Scripts/create_groom_bindings.py) fit each groom to this face and let it follow the face.
	const FString Character = Head->GetSkeletalMeshAsset()->GetName().Replace(TEXT("SKM_"), TEXT("")).Replace(TEXT("_FaceMesh"), TEXT(""));
	UGroomBindingAsset* Binding = LoadObject<UGroomBindingAsset>(nullptr, *FString::Printf(TEXT("/Game/MetaHumans/Bindings/%s/%s.%s"), *Character, *Name, *Name));
	Groom->AttachToComponent(Head, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Groom->SetRelativeTransform(FTransform::Identity);
	Groom->SetGroomAsset(Asset, Binding);

	static TSoftObjectPtr<UMaterialInterface> HairMaterial{FSoftObjectPath(HairMaterialPath)};
	if (UMaterialInterface* Parent = HairMaterial.LoadSynchronous())
	{
		for (int32 Slot = 0; Slot < Groom->GetNumMaterials(); Slot++)
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, Groom);
			M->SetScalarParameterValue(TEXT("hairMelanin"), Appearance.HairMelanin);
			M->SetScalarParameterValue(TEXT("hairRedness"), Appearance.HairRedness);
			Groom->SetMaterial(Slot, M);
		}
	}
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
