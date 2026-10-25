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
		KitLayers.Add(Layer);
	}

	StoneMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Astonia/Materials/MI_KitStone.MI_KitStone")));
	PlainMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Astonia/Materials/MI_KitPlain.MI_KitPlain")));
	GlowMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Astonia/Materials/M_KitGlow.M_KitGlow")));
}

void AAstoniaBuilding::ClearPieces()
{
	Modify();
	UMaterialInterface* Stone = StoneMaterial.LoadSynchronous();
	UMaterialInterface* Plain = PlainMaterial.LoadSynchronous();
	UMaterialInterface* Glow = GlowMaterial.LoadSynchronous();
	for (int32 Kit = 0; Kit < KitLayers.Num(); Kit++)
	{
		UHierarchicalInstancedStaticMeshComponent* Layer = KitLayers[Kit];
		Layer->Modify();
		Layer->ClearInstances();
		UMaterialInterface* Material = Kit == int32(EAstoniaKit::Glow) ? Glow : IsStone(Kit) ? Stone : Plain;
		if (Material) Layer->SetMaterial(0, Material);
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
	AddInstanceComponent(Light);
	Light->RegisterComponent();
}

void AAstoniaBuilding::FinishPieces()
{
	for (UHierarchicalInstancedStaticMeshComponent* Layer : KitLayers)
	{
		Layer->MarkRenderStateDirty();
	}
}

int32 AAstoniaBuilding::GetPieceCount() const
{
	int32 Count = 0;
	for (const UHierarchicalInstancedStaticMeshComponent* Layer : KitLayers)
	{
		Count += Layer->GetInstanceCount();
	}
	return Count;
}
