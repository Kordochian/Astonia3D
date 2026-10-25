#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "AstoniaAppearance.generated.h"

class UMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;

/** Where a piece goes on the body. One piece per slot. */
UENUM(BlueprintType)
enum class EAstoniaAppearanceSlot : uint8
{
	Hair,
	Beard,
	Eyes,
	Earrings,
	Necklace,
	RingLeft,
	RingRight,
	Bracelets,
	Shirt,
	Trousers,
	Shoes,
	Gloves,
	Cloak,
	Belt,
	Helmet,
	ChestArmor,
	ArmArmor,
	LegArmor,
	Boots,
	MainHand,
	OffHand,
};

/**
 * A wearable model: hair, jewellery, clothes, armour or a weapon (row of /Game/Characters/Appearance/DT_Pieces).
 * Skinned pieces (clothes, armour, hair rigged to the mannequin skeleton) follow the body's animation;
 * rigid pieces (rings, helmets, weapons) attach to a bone or socket.
 */
USTRUCT(BlueprintType)
struct ASTONIAGAME_API FAstoniaAppearancePieceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	EAstoniaAppearanceSlot Slot = EAstoniaAppearanceSlot::Shirt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FText DisplayName;

	/** Rigged to the UE5 mannequin skeleton; plays the body's animation (leader pose). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	/** Rigid model attached to AttachTo. Used when SkeletalMesh is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	/** Bone or socket of the body for rigid pieces, e.g. head, hand_r, hand_l, spine_05, pelvis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FName AttachTo = TEXT("head");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FTransform Offset;

	/** Colour sent to the piece's material as "Tint" (if it has one). Alpha 0 = keep the material's own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor Tint = FLinearColor(1.f, 1.f, 1.f, 0.f);

	/** Hair and beards take the character's hair colour, eyes the eye colour, instead of Tint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	bool bUseCharacterColor = false;
};

/** Everything that decides how a character looks: the default skin plus a piece per slot. */
USTRUCT(BlueprintType)
struct ASTONIAGAME_API FAstoniaAppearance
{
	GENERATED_BODY()

	/** Row of DT_Skins: skin tone and undergarments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FName Skin = TEXT("Light");

	/** Use the female body (Quinn) instead of the male (Manny). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	bool bFemale = false;

	/** Overrides of the skin's hair and eye colours; alpha 0 = use the skin's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor HairColor = FLinearColor(0.f, 0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FLinearColor EyeColor = FLinearColor(0.f, 0.f, 0.f, 0.f);

	/** Rows of DT_Pieces by slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	TMap<EAstoniaAppearanceSlot, FName> Pieces;

	// --- MetaHuman look (character creator) ---

	/** Skin brightness on the MetaHuman's baked skin: 1 = as assembled, lower = darker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia|MetaHuman")
	float SkinShade = 1.f;

	/** Skin colour saturation: 1 = as assembled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia|MetaHuman")
	float SkinSaturation = 1.f;

	/** MetaHuman grooms by name (folders in /MetaHumanCharacter/Optional/Grooms/GroomAssets); None = bald / none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia|MetaHuman")
	FName HairStyle = TEXT("Hair_S_Casual");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia|MetaHuman")
	FName Eyebrows = TEXT("Eyebrows_M_Natural");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia|MetaHuman")
	FName Beard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia|MetaHuman")
	FName Mustache;

	/** Hair colour for every groom, as MetaHuman sets it: melanin 0 = blond .. 1 = black, redness 0..1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia|MetaHuman")
	float HairMelanin = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia|MetaHuman")
	float HairRedness = 0.25f;
};

/** The MetaHuman groom kinds the creator offers, with the names of every style installed. */
UENUM(BlueprintType)
enum class EAstoniaGroomKind : uint8
{
	Hair,
	Eyebrows,
	Beards,
	Mustaches,
};

namespace AstoniaGrooms
{
	/** Style names of a kind (from MetaHuman Creator Core Data), sorted. */
	ASTONIAGAME_API const TArray<FName>& Styles(EAstoniaGroomKind Kind);

	/** "Hair_M_BobStraight" -> "Bob Straight (medium)". */
	ASTONIAGAME_API FText DisplayName(FName Style);
}

/**
 * Dresses the owner's body mesh: puts the default human skin on it and layers piece models on top.
 * Lives on the player and on NPCs; call Apply after changing Appearance, or use the setters.
 */
UCLASS(ClassGroup = (Astonia), meta = (BlueprintSpawnableComponent))
class ASTONIAGAME_API UAstoniaAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstoniaAppearanceComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FAstoniaAppearance Appearance;

	UPROPERTY(EditAnywhere, Category = "Astonia")
	TSoftObjectPtr<UDataTable> PieceTable;

	/**
	 * Smooth MetaHuman bodies and faces (Scripts/create_metahumans.py, assembled into /Game/MetaHumans).
	 * When set, the mannequin only drives the animation (hidden) and these follow it; when empty, the
	 * mannequin shows with the M_HumanSkin default skin.
	 */
	UPROPERTY(EditAnywhere, Category = "Astonia|MetaHuman")
	TSoftObjectPtr<USkeletalMesh> MaleMetaHumanBody;

	UPROPERTY(EditAnywhere, Category = "Astonia|MetaHuman")
	TSoftObjectPtr<USkeletalMesh> MaleMetaHumanFace;

	UPROPERTY(EditAnywhere, Category = "Astonia|MetaHuman")
	TSoftObjectPtr<USkeletalMesh> FemaleMetaHumanBody;

	UPROPERTY(EditAnywhere, Category = "Astonia|MetaHuman")
	TSoftObjectPtr<USkeletalMesh> FemaleMetaHumanFace;

	/** Rebuilds the body and every piece from Appearance. */
	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void Apply();

	/** True if the visible body was drawn in the last Seconds (the animation driver itself is hidden). */
	bool WasBodyRecentlyRendered(float Seconds) const;

	/** Shows or hides the hair, brows and beard (grooms are the costliest part of a character). */
	void SetGroomsVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void SetPiece(EAstoniaAppearanceSlot Slot, FName Piece);

	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void ClearPiece(EAstoniaAppearanceSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void SetSkin(FName Skin);

	/** Re-applies skin tone, eye colour, hair styles and colour without rebuilding the body. */
	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void RefreshLook();

protected:
	virtual void BeginPlay() override;

private:
	USkeletalMeshComponent* GetBody() const;
	void ApplyPiece(EAstoniaAppearanceSlot Slot);

	UPROPERTY(Transient)
	TMap<EAstoniaAppearanceSlot, TObjectPtr<UMeshComponent>> Spawned;

	void ApplyMetaHuman(USkeletalMeshComponent* Driver);

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MetaBody;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MetaFace;

	void ApplyMetaHumanLook();
	void ApplyGroom(EAstoniaGroomKind Kind, FName Style);

	UPROPERTY(Transient)
	TMap<EAstoniaGroomKind, TObjectPtr<class UGroomComponent>> Grooms;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMesh> MaleBody;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMesh> FemaleBody;
};
