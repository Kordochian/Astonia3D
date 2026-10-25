#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AstoniaSkin.generated.h"

class USkeletalMeshComponent;

/**
 * A look for the UE5 mannequin (Manny / Quinn): drives the mannequin material's paint parameters.
 * Rows live in /Game/Characters/Skins/DT_Skins (made by Scripts/create_skins.py, editable in the editor).
 */
USTRUCT(BlueprintType)
struct ASTONIAGAME_API FAstoniaSkinRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Main colour ("Paint Tint"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor Body = FLinearColor(0.5f, 0.5f, 0.5f);

	/** Emblem colour on the chest ("LogoTint"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor Trim = FLinearColor(0.8f, 0.7f, 0.3f);

	/** 0 = cloth or leather, 1 = metal plate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia", meta = (ClampMin = "0", ClampMax = "1"))
	float Metallic = 0.f;

	/** 0 = polished, 1 = matte. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia", meta = (ClampMin = "0", ClampMax = "1"))
	float Roughness = 0.7f;
};

namespace AstoniaSkin
{
	/** Path of the skin table. */
	ASTONIAGAME_API const TCHAR* TablePath();

	/** Applies a skin row to every material slot of a mannequin mesh. Does nothing when Skin is null. */
	ASTONIAGAME_API void Apply(USkeletalMeshComponent* Mesh, const FAstoniaSkinRow* Skin);

	/** Looks up a row in the skin table (loads it on first use). */
	ASTONIAGAME_API const FAstoniaSkinRow* Find(FName Skin);
}
