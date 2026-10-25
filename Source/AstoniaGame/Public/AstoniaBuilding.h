#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstoniaBuilding.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;

/** Placeholder kit pieces a building is made of. Each is one instanced layer with its own mesh and look. */
UENUM(BlueprintType)
enum class EAstoniaKit : uint8
{
	Stone,       // cube, dressed stone (courses on walls, flagstones on tops), collision
	StoneRound,  // cylinder, dressed stone, collision
	Trim,        // cube, stone without courses (string courses, sills, steps), collision
	Wood,        // cube, plain, collision
	Roof,        // cube, plain (roof slabs), collision
	RoofCone,    // cone, plain (tower roofs), collision
	Trunk,       // cylinder, plain, collision
	Leaves,      // sphere, plain, no collision
	Cloth,       // cube, plain, no collision (banners, awnings)
	Decor,       // cube, plain, no collision (slits, inlays, small things)
	DecorRound,  // cylinder, plain, no collision (mosaics, rings)
	Glow,        // sphere, emissive colour, no collision (torch flames)
	Ground,      // cube, plain, collision (earth and path floors)
	Rock,        // sphere, collision (boulders, ridges)
	Grass,       // cube floors by surface, collision
	Dirt,
	Cobble,
	Forest,
	Planks,
	Tiles,
	Flagstone,
	Plaster,     // cube, collision (rendered walls of half-timbered houses)
	Timber,      // cube, collision (plank walls)
	Blocker,     // cube, collision, never drawn (forest edges, gaps between trunks)
	TreeRound,   // Blender models (SM_Kit_<Layer>), no collision (trunks get a Blocker); sphere until imported
	TreeTall,
	TreeForest,
	Bush,
	StoneWall,     // modular building kit (Scripts/blender/make_buildings.py): one per wall tile,
	StoneWindow,   // material by style; no collision (builders add Blocker runs)
	StonePost,
	PlasterWall,
	PlasterWindow,
	PlasterPost,
	TimberWall,
	TimberWindow,
	TimberPost,
	RoofPanel,     // 2 x 2 m of shingles, +Y down the slope
	RoofRidge,
	Gable,         // unit triangle, scaled per roof end
	Count UMETA(Hidden)
};

/**
 * A building (fortress, house, bridge...) assembled from placeholder kit pieces: instanced cubes,
 * cylinders, cones and spheres, coloured per instance. A script writes the pieces (for Cameron's
 * fortress: Scripts/build_fortress.py, laid out from the original map), and the result is saved with
 * the level. New art replaces whole layers later (M9) without touching the layout.
 *
 * Sizes are in cm and are the full extent of the piece (a cylinder's X/Y size is its diameter).
 * A layer uses /Game/Astonia/Materials/Kit/MI_Kit_<Layer> when it exists (textured, world-aligned,
 * tinted by the piece colour), else the plain or stone placeholder material. A layer whose model exists
 * as /Game/Astonia/Meshes/SM_Kit_<Layer> (made in Blender, Scripts/blender) uses it instead of the
 * basic shape; each of its material slots takes MI_Kit_<SlotName> (e.g. Trunk, Leaves) when it exists.
 */
UCLASS()
class ASTONIAGAME_API AAstoniaBuilding : public AActor
{
	GENERATED_BODY()

public:
	AAstoniaBuilding();

	/** Removes every piece and light. */
	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void ClearPieces();

	/** Adds one piece centred on Center (actor space), rotated by Rotation. */
	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void AddPiece(EAstoniaKit Kit, FVector Center, FVector Size, FRotator Rotation, FLinearColor Color);

	/** Adds a point light (torch, brazier) in actor space. */
	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void AddLight(FVector Location, float Intensity = 2000.f, float Radius = 1200.f, FLinearColor Color = FLinearColor(1.f, 0.7f, 0.4f));

	/** Redraws the instances after a batch of AddPiece calls. */
	UFUNCTION(BlueprintCallable, Category = "Astonia")
	void FinishPieces();

	UFUNCTION(BlueprintCallable, Category = "Astonia")
	int32 GetPieceCount() const;

	/** Material for stone layers: PerInstanceCustomData 0-2 colour plus a world-space stone pattern. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TSoftObjectPtr<UMaterialInterface> StoneMaterial;

	/** Material for the other layers: PerInstanceCustomData 0-2 colour with light mottling. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TSoftObjectPtr<UMaterialInterface> PlainMaterial;

	/** Material for Glow: PerInstanceCustomData 0-2 as emissive colour. */
	UPROPERTY(EditAnywhere, Category = "Astonia|Look")
	TSoftObjectPtr<UMaterialInterface> GlowMaterial;

private:
	/** One layer per EAstoniaKit. Actors saved before a kit was added get the new layers back in
	 *  PostLoad (EnsureLayers). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> KitLayers;

	virtual void PostLoad() override;
	void EnsureLayers();
};
