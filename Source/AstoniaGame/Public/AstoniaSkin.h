#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AstoniaSkin.generated.h"

class USkeletalMeshComponent;

/**
 * A default body for the UE5 mannequin (Manny / Quinn): human skin tone with undergarments, drawn by
 * /Game/Characters/Skins/M_HumanSkin. Everything else a character wears (hair, clothes, armour,
 * jewellery, weapons) is a separate model from the appearance system (AstoniaAppearance.h).
 * Rows live in /Game/Characters/Skins/DT_Skins (Scripts/create_skins.py, editable in the editor).
 */
USTRUCT(BlueprintType)
struct ASTONIAGAME_API FAstoniaSkinRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor SkinTone = FLinearColor(0.72f, 0.50f, 0.38f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor UnderwearColor = FLinearColor(0.85f, 0.83f, 0.78f);

	/** Default eye colour for eye pieces (Eyes slot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor EyeColor = FLinearColor(0.35f, 0.25f, 0.15f);

	/** Default hair colour for hair pieces that use it (Hair, Beard slots). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor HairColor = FLinearColor(0.30f, 0.18f, 0.10f);
};

namespace AstoniaSkin
{
	/** Puts the human skin material on every slot of a mannequin body. Does nothing when Skin is null. */
	ASTONIAGAME_API void Apply(USkeletalMeshComponent* Body, const FAstoniaSkinRow* Skin, bool bFemale);

	/** Looks up a row in DT_Skins (loads the table on first use). */
	ASTONIAGAME_API const FAstoniaSkinRow* Find(FName Skin);
}
