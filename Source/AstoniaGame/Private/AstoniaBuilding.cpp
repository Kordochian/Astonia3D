#include "AstoniaBuilding.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName BuildingLightTag(TEXT("AstoniaBuildingLight"));

	struct FKitInfo
	{
		const TCHAR* Name;
		const TCHAR* Mesh;
		bool bCollision;
		const TCHAR* Model = nullptr;     // SM_Kit_<Model> (default: the layer name)
		const TCHAR* Material = nullptr;  // MI_Kit_<Material> for the model's "Wall" slot (default: the layer name)
	};

	// Indexed by EAstoniaKit. Engine basic shapes are 100 cm across and centred on their pivot.
	const FKitInfo KitInfo[] = {
		{TEXT("Stone"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("StoneRound"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), true},
		{TEXT("Trim"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Wood"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Roof"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("RoofCone"), TEXT("/Engine/BasicShapes/Cone.Cone"), true},
		{TEXT("Trunk"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), true},
		{TEXT("Leaves"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), false},
		{TEXT("Cloth"), TEXT("/Engine/BasicShapes/Cube.Cube"), false},
		{TEXT("Decor"), TEXT("/Engine/BasicShapes/Cube.Cube"), false},
		{TEXT("DecorRound"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), false},
		{TEXT("Glow"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), false},
		{TEXT("Ground"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Rock"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), true},
		{TEXT("Grass"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Dirt"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Cobble"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Forest"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Planks"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Tiles"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Flagstone"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Plaster"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Timber"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("Blocker"), TEXT("/Engine/BasicShapes/Cube.Cube"), true},
		{TEXT("TreeRound"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), false},
		{TEXT("TreeTall"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), false},
		{TEXT("TreeForest"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), false},
		{TEXT("Bush"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), false},
		{TEXT("StoneWall"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallStraight"), TEXT("Stone")},
		{TEXT("StoneWindow"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallWindow"), TEXT("Stone")},
		{TEXT("StonePost"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallPost"), TEXT("Stone")},
		{TEXT("PlasterWall"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallStraight"), TEXT("Plaster")},
		{TEXT("PlasterWindow"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallWindow"), TEXT("Plaster")},
		{TEXT("PlasterPost"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallPost"), TEXT("Plaster")},
		{TEXT("TimberWall"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallStraight"), TEXT("Timber")},
		{TEXT("TimberWindow"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallWindow"), TEXT("Timber")},
		{TEXT("TimberPost"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, TEXT("WallPost"), TEXT("Timber")},
		{TEXT("RoofPanel"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Roof")},
		{TEXT("RoofRidge"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Roof")},
		{TEXT("Gable"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Plaster")},
		{TEXT("DoorRickety"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Wood")},
		{TEXT("DoorPlank"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Wood")},
		{TEXT("DoorFancy"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Wood")},
		{TEXT("DoorMetal"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Metal")},
		{TEXT("FrameRickety"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Wood")},
		{TEXT("FramePlank"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Wood")},
		{TEXT("FrameFancy"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Trim")},
		{TEXT("FrameMetal"), TEXT("/Engine/BasicShapes/Cube.Cube"), false, nullptr, TEXT("Trim")},
	};
	static_assert(UE_ARRAY_COUNT(KitInfo) == int32(EAstoniaKit::Count), "KitInfo must list every EAstoniaKit");

	bool IsStone(int32 Kit)
	{
		return Kit == int32(EAstoniaKit::Stone) || Kit == int32(EAstoniaKit::StoneRound) || Kit == int32(EAstoniaKit::Trim);
	}
}

AAstoniaBuilding::AAstoniaBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	for (int32 Kit = 0; Kit < int32(EAstoniaKit::Count); Kit++)
	{
		const FKitInfo& Info = KitInfo[Kit];
		UHierarchicalInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Info.Name);
		Layer->SetupAttachment(RootComponent);
		Layer->NumCustomDataFloats = 3;
		Layer->SetCollisionProfileName(Info.bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		Layer->SetCastShadow(Kit != int32(EAstoniaKit::Glow));
		ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(Info.Mesh);
		if (Mesh.Succeeded()) Layer->SetStaticMesh(Mesh.Object);
		if (Kit == int32(EAstoniaKit::Blocker)) Layer->SetVisibility(false);
		// Performance: floors (slabs flush with the ground) cast no shadows; small decoration fades out.
		const bool bFloor = Kit == int32(EAstoniaKit::Ground) || (Kit >= int32(EAstoniaKit::Grass) && Kit <= int32(EAstoniaKit::Flagstone));
		if (bFloor || Kit == int32(EAstoniaKit::Blocker)) Layer->SetCastShadow(false);
		if (Kit == int32(EAstoniaKit::Decor) || Kit == int32(EAstoniaKit::DecorRound) || Kit == int32(EAstoniaKit::Glow) || Kit == int32(EAstoniaKit::Cloth))
		{
			Layer->SetCullDistances(7000, 9000);
		}
		else if (Kit == int32(EAstoniaKit::Rock))
		{
			Layer->SetCullDistances(18000, 22000);
		}
		KitLayers.Add(Layer);
	}

	StoneMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Astonia/Materials/MI_KitStone.MI_KitStone")));
	PlainMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Astonia/Materials/MI_KitPlain.MI_KitPlain")));
	GlowMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Astonia/Materials/M_KitGlow.M_KitGlow")));
}

void AAstoniaBuilding::PostLoad()
{
	Super::PostLoad();
	EnsureLayers();
}

void AAstoniaBuilding::EnsureLayers()
{
	// The constructor made every layer; an older save only listed the kits that existed then.
	KitLayers.SetNum(int32(EAstoniaKit::Count));
	for (int32 Kit = 0; Kit < KitLayers.Num(); Kit++)
	{
		if (!KitLayers[Kit]) KitLayers[Kit] = FindObject<UHierarchicalInstancedStaticMeshComponent>(this, KitInfo[Kit].Name);
	}
}

void AAstoniaBuilding::ClearPieces()
{
	EnsureLayers();
	Modify();
	UMaterialInterface* Stone = StoneMaterial.LoadSynchronous();
	UMaterialInterface* Plain = PlainMaterial.LoadSynchronous();
	UMaterialInterface* Glow = GlowMaterial.LoadSynchronous();
	for (int32 Kit = 0; Kit < KitLayers.Num(); Kit++)
	{
		UHierarchicalInstancedStaticMeshComponent* Layer = KitLayers[Kit];
		if (!Layer) continue;
		Layer->Modify();
		Layer->ClearInstances();
		// A Blender model for this layer replaces the basic shape when it has been imported.
		const TCHAR* ModelName = KitInfo[Kit].Model ? KitInfo[Kit].Model : KitInfo[Kit].Name;
		const FString Model = FString::Printf(TEXT("/Game/Astonia/Meshes/SM_Kit_%s.SM_Kit_%s"), ModelName, ModelName);
		if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Model, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			if (Layer->GetStaticMesh() != Mesh) Layer->SetStaticMesh(Mesh);
		}
		auto KitMaterial = [](const FString& Name)
		{
			const FString Path = FString::Printf(TEXT("/Game/Astonia/Materials/Kit/MI_Kit_%s.MI_Kit_%s"), *Name, *Name);
			return LoadObject<UMaterialInterface>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		};
		// Textured kit material by convention, else the placeholder look; model slots by slot name.
		UMaterialInterface* Material = KitMaterial(KitInfo[Kit].Material ? KitInfo[Kit].Material : KitInfo[Kit].Name);
		if (!Material) Material = Kit == int32(EAstoniaKit::Glow) ? Glow : IsStone(Kit) ? Stone : Plain;
		const TArray<FName> Slots = Layer->GetMaterialSlotNames();
		for (int32 Slot = 0; Slot < FMath::Max(1, Slots.Num()); Slot++)
		{
			// A model's "Wall" slot takes the layer's material; other named slots MI_Kit_<SlotName>.
			const bool bNamed = Slots.IsValidIndex(Slot) && (Slots.Num() > 1 || KitInfo[Kit].Model || KitInfo[Kit].Material) && Slots[Slot] != TEXT("Wall");
			UMaterialInterface* SlotMaterial = bNamed ? KitMaterial(Slots[Slot].ToString()) : nullptr;
			if (SlotMaterial || Material) Layer->SetMaterial(Slot, SlotMaterial ? SlotMaterial : Material);
		}
	}
	TArray<UActorComponent*> OldLights = GetComponentsByTag(UPointLightComponent::StaticClass(), BuildingLightTag);
	for (UActorComponent* Old : OldLights)
	{
		RemoveInstanceComponent(Old);
		Old->DestroyComponent();
	}
}

void AAstoniaBuilding::AddPiece(EAstoniaKit Kit, FVector Center, FVector Size, FRotator Rotation, FLinearColor Color)
{
	if (!KitLayers.IsValidIndex(int32(Kit))) return;
	UHierarchicalInstancedStaticMeshComponent* Layer = KitLayers[int32(Kit)];
	if (!Layer) return;
	const int32 Index = Layer->AddInstance(FTransform(Rotation, Center, Size / 100.f));
	Layer->SetCustomDataValue(Index, 0, Color.R, false);
	Layer->SetCustomDataValue(Index, 1, Color.G, false);
	Layer->SetCustomDataValue(Index, 2, Color.B, false);
}

void AAstoniaBuilding::AddLight(FVector Location, float Intensity, float Radius, FLinearColor Color)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transactional);
	Light->ComponentTags.Add(BuildingLightTag);
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(Location);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(Radius);
	Light->SetLightColor(Color);
	Light->SetCastShadows(false);
	// Torches only matter near the viewer: fade them out instead of shading the whole village.
	Light->MaxDrawDistance = 5000.f;
	Light->MaxDistanceFadeRange = 1500.f;
	AddInstanceComponent(Light);
	Light->RegisterComponent();
}

void AAstoniaBuilding::FinishPieces()
{
	for (UHierarchicalInstancedStaticMeshComponent* Layer : KitLayers)
	{
		if (Layer) Layer->MarkRenderStateDirty();
	}
}

int32 AAstoniaBuilding::GetPieceCount() const
{
	int32 Count = 0;
	for (const UHierarchicalInstancedStaticMeshComponent* Layer : KitLayers)
	{
		if (Layer) Count += Layer->GetInstanceCount();
	}
	return Count;
}
