#include "AstoniaZoneActor.h"
#include "AstoniaCore.h"
#include "AstoniaDataRows.h"
#include "AstoniaTextFormat.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName LightTag(TEXT("AstoniaZoneLight"));

	// Placeholder palettes. Colours only say "these tiles share a sprite family", nothing more.
	const FLinearColor GroundPalette[] = {
		{0.20f, 0.33f, 0.12f}, {0.26f, 0.38f, 0.15f}, {0.33f, 0.29f, 0.20f}, {0.40f, 0.36f, 0.27f},
		{0.30f, 0.30f, 0.30f}, {0.38f, 0.37f, 0.34f}, {0.22f, 0.28f, 0.16f}, {0.45f, 0.40f, 0.30f},
	};
	const FLinearColor WallPalette[] = {
		{0.45f, 0.43f, 0.40f}, {0.35f, 0.30f, 0.25f}, {0.55f, 0.52f, 0.48f}, {0.18f, 0.30f, 0.14f},
		{0.40f, 0.33f, 0.26f}, {0.28f, 0.27f, 0.27f},
	};
	const FLinearColor IndoorFloor(0.42f, 0.30f, 0.20f);
	const FLinearColor Water(0.10f, 0.22f, 0.40f);
	const FLinearColor Friendly(0.15f, 0.35f, 0.85f);
	const FLinearColor Hostile(0.85f, 0.15f, 0.12f);

	template <int32 N>
	FLinearColor Pick(const FLinearColor (&Palette)[N], int32 Key)
	{
		return Palette[(uint32(Key) * 2654435761u >> 16) % N];
	}

	FLinearColor ItemColor(EAstoniaZoneItemKind Kind)
	{
		switch (Kind)
		{
		case EAstoniaZoneItemKind::Door: return {0.35f, 0.20f, 0.08f};
		case EAstoniaZoneItemKind::Light: return {1.0f, 0.75f, 0.30f};
		case EAstoniaZoneItemKind::Container: return {0.60f, 0.45f, 0.10f};
		case EAstoniaZoneItemKind::Teleport: return {0.55f, 0.20f, 0.75f};
		case EAstoniaZoneItemKind::Blocker: return {0.50f, 0.50f, 0.55f};
		default: return {0.80f, 0.80f, 0.80f};
		}
	}

	void AddInstance(UHierarchicalInstancedStaticMeshComponent* Layer, const FTransform& T, const FLinearColor& C)
	{
		const int32 Index = Layer->AddInstance(T);
		Layer->SetCustomDataValue(Index, 0, C.R, false);
		Layer->SetCustomDataValue(Index, 1, C.G, false);
		Layer->SetCustomDataValue(Index, 2, C.B, false);
	}

	/** Rows of a zone-specific copy are named "<template>@<zone>"; fall back to the plain name. */
	template <typename RowType>
	const RowType* FindTemplate(const UDataTable* Table, const FString& Name, FName Zone, FName& OutRow)
	{
		if (!Table) return nullptr;
		OutRow = FName(*FString::Printf(TEXT("%s@%s"), *Name, *Zone.ToString()));
		if (const RowType* Row = Table->FindRow<RowType>(OutRow, TEXT(""), false)) return Row;
		OutRow = FName(*Name);
		return Table->FindRow<RowType>(OutRow, TEXT(""), false);
	}

	EAstoniaZoneItemKind Classify(const FAstoniaItemRow* Row)
	{
		if (!Row) return EAstoniaZoneItemKind::Other;
		if (Row->HasFlag(TEXT("IF_DOOR"))) return EAstoniaZoneItemKind::Door;
		for (const FAstoniaStatModifier& M : Row->Modifiers)
		{
			if (M.Stat == TEXT("V_LIGHT") && M.Value > 0 && !M.bRequirement) return EAstoniaZoneItemKind::Light;
		}
		if (Row->Driver == 10) return EAstoniaZoneItemKind::Teleport;  // teleport driver (castle/knight_teleport)
		if (Row->Driver == 5) return EAstoniaZoneItemKind::Container;  // chest driver
		if (Row->HasFlag(TEXT("IF_MOVEBLOCK"))) return EAstoniaZoneItemKind::Blocker;
		return EAstoniaZoneItemKind::Other;
	}
}

AAstoniaZoneActor::AAstoniaZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Ground = MakeLayer(TEXT("Ground"), TEXT("/Engine/BasicShapes/Cube.Cube"), true);
	Walls = MakeLayer(TEXT("Walls"), TEXT("/Engine/BasicShapes/Cube.Cube"), true);
	Props = MakeLayer(TEXT("Props"), TEXT("/Engine/BasicShapes/Cone.Cone"), false);
	ItemMeshes = MakeLayer(TEXT("Items"), TEXT("/Engine/BasicShapes/Cube.Cube"), true);
	NpcMarkers = MakeLayer(TEXT("NpcMarkers"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), false);

	ItemTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Astonia/Data/DT_Items.DT_Items")));
	CharacterTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Astonia/Data/DT_Characters.DT_Characters")));
	PlaceholderMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Astonia/Materials/M_Placeholder.M_Placeholder")));
}

UHierarchicalInstancedStaticMeshComponent* AAstoniaZoneActor::MakeLayer(FName Name, const TCHAR* MeshPath, bool bCollision)
{
	UHierarchicalInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
	Layer->SetupAttachment(RootComponent);
	Layer->NumCustomDataFloats = 3;
	Layer->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(MeshPath);
	if (Mesh.Succeeded()) Layer->SetStaticMesh(Mesh.Object);
	return Layer;
}

FVector AAstoniaZoneActor::TileToLocal(int32 X, int32 Y)
{
	return FVector((X + 0.5f) * AstoniaTileSize, (Y + 0.5f) * AstoniaTileSize, 0.f);
}

FVector AAstoniaZoneActor::TileToWorld(int32 X, int32 Y) const
{
	return GetActorTransform().TransformPosition(TileToLocal(X, Y));
}

#if WITH_EDITOR
void AAstoniaZoneActor::RebuildFromMap()
{
	FString Text, Error;
	FAstoniaMap Map;
	if (!FFileHelper::LoadFileToString(Text, *MapFile) || !AstoniaTextFormat::ParseMap(Text, Map, Error))
	{
		UE_LOG(LogAstoniaCore, Error, TEXT("%s: cannot build zone from %s: %s"), *GetName(), *MapFile, *Error);
		return;
	}

	Modify();
	const UDataTable* ItemsDT = ItemTable.LoadSynchronous();
	const UDataTable* CharsDT = CharacterTable.LoadSynchronous();
	if (UMaterialInterface* Material = PlaceholderMaterial.LoadSynchronous())
	{
		for (UHierarchicalInstancedStaticMeshComponent* Layer : {Ground.Get(), Walls.Get(), Props.Get(), ItemMeshes.Get(), NpcMarkers.Get()})
		{
			Layer->SetMaterial(0, Material);
		}
	}

	for (UHierarchicalInstancedStaticMeshComponent* Layer : {Ground.Get(), Walls.Get(), Props.Get(), ItemMeshes.Get(), NpcMarkers.Get()})
	{
		Layer->Modify();
		Layer->ClearInstances();
	}
	TArray<UActorComponent*> OldLights = GetComponentsByTag(UPointLightComponent::StaticClass(), LightTag);
	for (UActorComponent* Old : OldLights)
	{
		RemoveInstanceComponent(Old);
		Old->DestroyComponent();
	}
	Items.Reset();
	Spawns.Reset();

	// Engine basic shapes are 100 cm across and centred on their pivot.
	const float S = AstoniaTileSize / 100.f;
	int32 MissingItems = 0, MissingChars = 0;

	for (int32 Y = 0; Y < Map.Size; Y++)
	{
		for (int32 X = 0; X < Map.Size; X++)
		{
			const FAstoniaMapTile& Tile = Map.At(X, Y);
			const FVector P = TileToLocal(X, Y);
			const bool bBlocked = EnumHasAnyFlags(Tile.Flags, EAstoniaMapFlag::MoveBlock);

			if (Tile.GroundSprite || Tile.ForegroundSprite || !Tile.Item.IsEmpty() || !Tile.Character.IsEmpty() || bBlocked)
			{
				FLinearColor C = Pick(GroundPalette, Tile.GroundSprite / 100);
				if (EnumHasAnyFlags(Tile.Flags, EAstoniaMapFlag::Underwater)) C = Water;
				else if (EnumHasAnyFlags(Tile.Flags, EAstoniaMapFlag::Indoors)) C = IndoorFloor;
				AddInstance(Ground, FTransform(FQuat::Identity, P - FVector(0, 0, 10.f), FVector(S, S, 0.2f)), C);
			}

			if (bBlocked)
			{
				AddInstance(Walls, FTransform(FQuat::Identity, P + FVector(0, 0, WallHeight * 0.5f), FVector(S, S, WallHeight / 100.f)),
					Pick(WallPalette, Tile.ForegroundSprite / 1000));
			}
			else if (Tile.ForegroundSprite)
			{
				AddInstance(Props, FTransform(FQuat::Identity, P + FVector(0, 0, 50.f), FVector(0.8f, 0.8f, 1.f)),
					Pick(WallPalette, Tile.ForegroundSprite / 1000));
			}

			if (!Tile.Item.IsEmpty())
			{
				FName RowName;
				const FAstoniaItemRow* Row = FindTemplate<FAstoniaItemRow>(ItemsDT, Tile.Item, Zone, RowName);
				MissingItems += Row == nullptr;
				const EAstoniaZoneItemKind Kind = Classify(Row);
				Items.Add({RowName, Kind, FIntPoint(X, Y)});

				FVector Size(0.6f, 0.6f, 0.6f);
				switch (Kind)
				{
				case EAstoniaZoneItemKind::Door: Size = FVector(S * 0.9f, S * 0.9f, 2.6f); break;
				case EAstoniaZoneItemKind::Light: Size = FVector(0.2f, 0.2f, 0.6f); break;
				case EAstoniaZoneItemKind::Container: Size = FVector(1.0f, 0.7f, 0.7f); break;
				case EAstoniaZoneItemKind::Teleport: Size = FVector(S * 0.8f, S * 0.8f, 0.1f); break;
				default: break;
				}
				// Wall torches sit on a blocked tile: hang them on the side that faces an open tile.
				FVector Offset = FVector::ZeroVector;
				if (Kind == EAstoniaZoneItemKind::Light && bBlocked)
				{
					for (const FIntPoint D : {FIntPoint(0, 1), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(-1, 0)})
					{
						const int32 NX = X + D.X, NY = Y + D.Y;
						if (NX >= 0 && NY >= 0 && NX < Map.Size && NY < Map.Size && !EnumHasAnyFlags(Map.At(NX, NY).Flags, EAstoniaMapFlag::MoveBlock))
						{
							Offset = FVector(D.X, D.Y, 0) * (AstoniaTileSize * 0.5f + 15.f);
							break;
						}
					}
				}
				const float Lift = Kind == EAstoniaZoneItemKind::Light ? 200.f : Size.Z * 50.f;
				AddInstance(ItemMeshes, FTransform(FQuat::Identity, P + Offset + FVector(0, 0, Lift), Size), ItemColor(Kind));

				if (Kind == EAstoniaZoneItemKind::Light)
				{
					UPointLightComponent* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transactional);
					Light->ComponentTags.Add(LightTag);
					Light->SetupAttachment(RootComponent);
					Light->SetRelativeLocation(P + Offset * 1.3f + FVector(0, 0, 230.f));
					Light->SetIntensity(1500.f);
					Light->SetAttenuationRadius(900.f);
					Light->SetLightColor(FLinearColor(1.f, 0.7f, 0.4f));
					Light->SetCastShadows(false);
					AddInstanceComponent(Light);
					Light->RegisterComponent();
				}
			}

			if (!Tile.Character.IsEmpty())
			{
				FName RowName;
				const FAstoniaCharacterRow* Row = FindTemplate<FAstoniaCharacterRow>(CharsDT, Tile.Character, Zone, RowName);
				MissingChars += Row == nullptr;
				const bool bHostile = Row && !Row->HasFlag(TEXT("CF_NOATTACK")) && !Row->HasFlag(TEXT("CF_IMMORTAL"));
				Spawns.Add({RowName, FIntPoint(X, Y), bHostile});
				AddInstance(NpcMarkers, FTransform(FQuat::Identity, P + FVector(0, 0, 95.f), FVector(0.8f, 0.8f, 1.9f)),
					bHostile ? Hostile : Friendly);
			}
		}
	}

	for (UHierarchicalInstancedStaticMeshComponent* Layer : {Ground.Get(), Walls.Get(), Props.Get(), ItemMeshes.Get(), NpcMarkers.Get()})
	{
		Layer->MarkRenderStateDirty();
	}

	UE_LOG(LogAstoniaCore, Display, TEXT("%s: built from %s: %d ground, %d walls, %d props, %d items (%d unknown), %d NPCs (%d unknown)"),
		*GetName(), *MapFile, Ground->GetInstanceCount(), Walls->GetInstanceCount(), Props->GetInstanceCount(),
		Items.Num(), MissingItems, Spawns.Num(), MissingChars);
}
#endif
