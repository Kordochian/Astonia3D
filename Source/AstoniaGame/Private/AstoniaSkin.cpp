#include "AstoniaSkin.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

const TCHAR* AstoniaSkin::TablePath()
{
	return TEXT("/Game/Characters/Skins/DT_Skins.DT_Skins");
}

void AstoniaSkin::Apply(USkeletalMeshComponent* Mesh, const FAstoniaSkinRow* Skin)
{
	if (!Mesh || !Skin) return;

	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); Slot++)
	{
		if (UMaterialInstanceDynamic* M = Mesh->CreateDynamicMaterialInstance(Slot))
		{
			M->SetVectorParameterValue(TEXT("Paint Tint"), Skin->Body);
			M->SetVectorParameterValue(TEXT("LogoTint"), Skin->Trim);
			M->SetScalarParameterValue(TEXT("MetalPaintMetallic"), Skin->Metallic);
			M->SetScalarParameterValue(TEXT("MetalPaintRoughness"), Skin->Roughness);
		}
	}
}

const FAstoniaSkinRow* AstoniaSkin::Find(FName Skin)
{
	static TSoftObjectPtr<UDataTable> Table{FSoftObjectPath(TablePath())};
	const UDataTable* DT = Table.LoadSynchronous();
	return DT && !Skin.IsNone() ? DT->FindRow<FAstoniaSkinRow>(Skin, TEXT("AstoniaSkin"), false) : nullptr;
}
