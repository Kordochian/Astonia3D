#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstoniaNpc.h"
#include "AstoniaZoneActor.generated.h"

class UDataTable;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EAstoniaZoneItemKind : uint8
{
	Other,
	Door,
	Light,
	Container,
	Teleport,
	Blocker,
};

/** A map item placed in the zone (it= in the .map file). */
USTRUCT(BlueprintType)
struct FAstoniaZoneItem
{
	GENERATED_BODY()

	/** DT_Items row name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Item;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	EAstoniaZoneItemKind Kind = EAstoniaZoneItemKind::Other;

	/** Tile in the original map. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FIntPoint Tile = FIntPoint::ZeroValue;

	/** Where it ended up, in actor space (floor height). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FVector Location = FVector::ZeroVector;
};

/** An NPC placed in the zone (ch= in the .map file). Spawned as a real NPC in M3. */
USTRUCT(BlueprintType)
struct FAstoniaZoneSpawn
{
	GENERATED_BODY()

	/** DT_Characters row name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FIntPoint Tile = FIntPoint::ZeroValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FVector Location = FVector::ZeroVector;

	/** Attackable (no CF_NOATTACK / CF_IMMORTAL). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	bool bHostile = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	EAstoniaNpcRole Role = EAstoniaNpcRole::Monster;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString DisplayName;

	/** CF_FEMALE: uses the female body. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	bool bFemale = false;
};

/**
 * Moves one walkable section of the original map (an area the 2D game reached through a teleport
 * hole) underneath its entrance, so it becomes a basement or cavern joined by a lift shaft.
 */
USTRUCT(BlueprintType)
struct FAstoniaSectionPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FString Label;

	/** Any walkable tile of the section; the lift arrives here (the original teleport target). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FIntPoint EntryTile = FIntPoint::ZeroValue;

	/** Original tile, in the parent's map coordinates, where the shaft starts (the original hole). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FIntPoint ColumnTile = FIntPoint::ZeroValue;

	/** Index of the placement the shaft starts in; -1 = the surface. Must be earlier in the list. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	int32 Parent = -1;

	/** Floor depth below the surface in cm. Keep levels at least 600 apart. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	float Depth = 600.f;

	/** Hide the shaft entrance behind a secret door (a wall block that sinks when searched). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	bool bSecretDoor = false;
};

/** Taller walls for part of the surface (fortress walls, towers). Later entries win. */
USTRUCT(BlueprintType)
struct FAstoniaHeightRegion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FIntPoint Min = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	FIntPoint Max = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astonia")
	float WallHeight = 800.f;
};

/** A built lift shaft, for the script that places the lift and secret door actors. */
USTRUCT(BlueprintType)
struct FAstoniaShaft
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString Label;

	/** Top floor centre, actor space. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FVector Top = FVector::ZeroVector;

	/** How far the platform travels down in cm (positive); it stops resting on the lower floor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	float Drop = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	bool bSecretDoor = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FLinearColor WallColor = FLinearColor::Gray;
};

/**
 * One original area rebuilt as placeholder 3D geometry: ground tiles, walls with collision, roofs,
 * props, map items and NPC spawn markers, all as instanced meshes coloured per instance. Sections
 * the 2D game reached through teleport holes can be moved underground (Placements). Press
 * "Rebuild From Map" in the Details panel (or call it from a script) after the .map or the
 * settings change; the result is saved with the level, so the game never reads the original files.
 *
 * Layout: tile (x, y) covers [x, x+1) * AstoniaTileSize on X and the same on Y, surface floor at
 * Z = 0 of the actor. Astonia's y axis (south) is Unreal +Y.
 */
UCLASS()
class ASTONIAGAME_API AAstoniaZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AAstoniaZoneActor();

	/** Spawns the 3D NPCs (CharacterRoles) from Spawns. */
	virtual void BeginPlay() override;

	/** Centre of a tile in actor space, at surface floor height. */
	static FVector TileToLocal(int32 X, int32 Y);

	/** Centre of a tile in world space, at surface floor height. */
	UFUNCTION(BlueprintCallable, Category = "Astonia")
	FVector TileToWorld(int32 X, int32 Y) const;

#if WITH_EDITOR
	/** Reads MapFile and regenerates every instance, light, item, spawn and shaft. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Astonia")
	void RebuildFromMap();
#endif

	/** Original .map file (absolute path) this zone was built from. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Source")
	FString MapFile;

	/** Zone folder of the map, used to prefer "<template>@<zone>" rows (e.g. "1" for Cameron). */
	UPROPERTY(EditAnywhere, Category = "Astonia|Source")
	FName Zone;

	UPROPERTY(EditAnywhere, Category = "Astonia|Source")
	TSoftObjectPtr<UDataTable> ItemTable;

	UPROPERTY(EditAnywhere, Category = "Astonia|Source")
	TSoftObjectPtr<UDataTable> CharacterTable;

	/** Material that reads PerInstanceCustomData 0-2 as base colour. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TSoftObjectPtr<UMaterialInterface> PlaceholderMaterial;

	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	float WallHeight = 300.f;

	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TArray<FAstoniaHeightRegion> HeightRegions;

	/** Roof slabs over indoor tiles on the surface. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	bool bBuildRoofs = true;

	UPROPERTY(EditAnywhere, Category = "Astonia|Layout")
	TArray<FAstoniaSectionPlacement> Placements;

	/** Where a new character starts, in tiles (server: 126,179 in Cameron, the castle courtyard). */
	UPROPERTY(EditAnywhere, Category = "Astonia|Layout")
	FIntPoint StartTile = FIntPoint(126, 179);

	/** NPCs of these roles appear as 3D characters (AAstoniaNpc) at play; the rest stay markers. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Layout")
	TArray<EAstoniaNpcRole> CharacterRoles = {EAstoniaNpcRole::QuestGiver, EAstoniaNpcRole::Merchant, EAstoniaNpcRole::Thief};

	/** Character templates (name prefixes) that count as thieves: Lydia's thieves in the west woods. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Layout")
	TArray<FString> ThiefTemplates = {TEXT("wood_thief")};

	/** Skins (DT_Skins rows) for named characters by template, e.g. knight_castle_mage -> MageRobe. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TMap<FName, FName> CharacterSkins;

	/** Skins other characters pick from (by template name, so a character always looks the same). */
	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TArray<FName> QuestGiverSkins = {TEXT("Peasant"), TEXT("TownsfolkBlue"), TEXT("TownsfolkRed"), TEXT("TownsfolkGreen")};

	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TArray<FName> MerchantSkins = {TEXT("Merchant")};

	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TArray<FName> ThiefSkins = {TEXT("ThiefLeather")};

	UPROPERTY(VisibleAnywhere, Category = "Astonia|Content")
	TArray<FAstoniaZoneItem> Items;

	UPROPERTY(VisibleAnywhere, Category = "Astonia|Content")
	TArray<FAstoniaZoneSpawn> Spawns;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia|Content")
	TArray<FAstoniaShaft> Shafts;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Walls;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Roofs;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Props;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ItemMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> NpcMarkers;

	UHierarchicalInstancedStaticMeshComponent* MakeLayer(FName Name, const TCHAR* MeshPath, bool bCollision);
	TArray<UHierarchicalInstancedStaticMeshComponent*> Layers() const;
};
