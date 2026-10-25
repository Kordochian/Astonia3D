#include "AstoniaSkin.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* SkinTablePath = TEXT("/Game/Characters/Skins/DT_Skins.DT_Skins");
	const TCHAR* SkinMaterialPath = TEXT("/Game/Characters/Skins/M_HumanSkin.M_HumanSkin");
}

void AstoniaSkin::Apply(USkeletalMeshComponent* Body, const FAstoniaSkinRow* Skin, bool bFemale)
{
	static TSoftObjectPtr<UMaterialInterface> Material{FSoftObjectPath(SkinMaterialPath)};
	UMaterialInterface* Parent = Material.LoadSynchronous();
	if (!Body || !Skin || !Parent) return;

	// Rest-pose heights in cm from the feet; Manny and Quinn are both 180 cm, Quinn is narrower.
	for (int32 Slot = 0; Slot < Body->GetNumMaterials(); Slot++)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, Body);
		M->SetVectorParameterValue(TEXT("SkinTone"), Skin->SkinTone);
		M->SetVectorParameterValue(TEXT("UnderwearColor"), Skin->UnderwearColor);
		M->SetScalarParameterValue(TEXT("Female"), bFemale ? 1.f : 0.f);
		M->SetScalarParameterValue(TEXT("HalfWidth"), bFemale ? 19.f : 21.f);
		Body->SetMaterial(Slot, M);
	}
}

const FAstoniaSkinRow* AstoniaSkin::Find(FName Skin)
{
	static TSoftObjectPtr<UDataTable> Table{FSoftObjectPath(SkinTablePath)};
	const UDataTable* DT = Table.LoadSynchronous();
	return DT && !Skin.IsNone() ? DT->FindRow<FAstoniaSkinRow>(Skin, TEXT("AstoniaSkin"), false) : nullptr;
}
