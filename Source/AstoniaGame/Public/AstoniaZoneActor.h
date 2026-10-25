#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FIntPoint Tile = FIntPoint::ZeroValue;
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

	/** Attackable (no CF_NOATTACK / CF_IMMORTAL). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	bool bHostile = false;
};

/**
 * One original area rebuilt as placeholder 3D geometry: ground tiles, walls with collision, props,
 * map items and NPC spawn markers, all as instanced meshes coloured per instance. Press
 * "Rebuild From Map" in the Details panel (or call it from a script) after the .map changes;
 * the result is saved with the level, so the game never reads the original files.
 *
 * Layout: tile (x, y) covers [x, x+1) * AstoniaTileSize on X and the same on Y, ground top at Z = 0
 * of the actor. Astonia's y axis (south) is Unreal +Y.
 */
UCLASS()
class ASTONIAGAME_API AAstoniaZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AAstoniaZoneActor();

	/** Centre of a tile in actor space, at ground height. */
	static FVector TileToLocal(int32 X, int32 Y);

	/** Centre of a tile in world space, at ground height. */
	UFUNCTION(BlueprintCallable, Category = "Astonia")
	FVector TileToWorld(int32 X, int32 Y) const;

#if WITH_EDITOR
	/** Reads MapFile and regenerates every instance, light, item and spawn. */
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

	UPROPERTY(VisibleAnywhere, Category = "Astonia|Content")
	TArray<FAstoniaZoneItem> Items;

	UPROPERTY(VisibleAnywhere, Category = "Astonia|Content")
	TArray<FAstoniaZoneSpawn> Spawns;

	/** Where a new character starts, in tiles (server: 126,179 in Cameron). */
	UPROPERTY(EditAnywhere, Category = "Astonia|Content")
	FIntPoint StartTile = FIntPoint(126, 179);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Walls;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Props;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ItemMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> NpcMarkers;

	UHierarchicalInstancedStaticMeshComponent* MakeLayer(FName Name, const TCHAR* MeshPath, bool bCollision);
};
