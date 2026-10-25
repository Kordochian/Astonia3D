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
	const FLinearColor CaveFloor(0.22f, 0.20f, 0.18f);
	const FLinearColor Water(0.10f, 0.22f, 0.40f);
	const FLinearColor Roof(0.30f, 0.14f, 0.10f);
	const FLinearColor ShaftWall(0.25f, 0.24f, 0.22f);
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
		if (Row->Driver == 10) return EAstoniaZoneItemKind::Teleport;  // IDR_TELEPORT (drvlib.h)
		if (Row->Driver == 5) return EAstoniaZoneItemKind::Container;  // chest driver
		if (Row->HasFlag(TEXT("IF_MOVEBLOCK"))) return EAstoniaZoneItemKind::Blocker;
		return EAstoniaZoneItemKind::Other;
	}

	/** IDR_TELEPORT data: x, y, area as little-endian 16-bit values; area 0 = same area. */
	int32 TeleportArea(const FAstoniaItemRow& Row)
	{
		TArray<uint8> Bytes;
		Bytes.SetNumZeroed(6);
		HexToBytes(Row.DriverArg.Left(12), Bytes.GetData());
		return Bytes[4] | Bytes[5] << 8;
	}

	/** One copy of the original map, moved by Offset tiles and placed at floor height Z. */
	struct FLevel
	{
		FIntPoint Offset = FIntPoint::ZeroValue;
		float Z = 0.f;
		int32 Section = INDEX_NONE;
	};
}

AAstoniaZoneActor::AAstoniaZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Ground = MakeLayer(TEXT("Ground"), TEXT("/Engine/BasicShapes/Cube.Cube"), true);
	Walls = MakeLayer(TEXT("Walls"), TEXT("/Engine/BasicShapes/Cube.Cube"), true);
	Roofs = MakeLayer(TEXT("Roofs"), TEXT("/Engine/BasicShapes/Cube.Cube"), true);
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

TArray<UHierarchicalInstancedStaticMeshComponent*> AAstoniaZoneActor::Layers() const
{
	return {Ground.Get(), Walls.Get(), Roofs.Get(), Props.Get(), ItemMeshes.Get(), NpcMarkers.Get()};
}

void AAstoniaZoneActor::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (const FAstoniaZoneSpawn& Spawn : Spawns)
	{
		if (!CharacterRoles.Contains(Spawn.Role)) continue;
		const FVector Location = GetActorTransform().TransformPosition(Spawn.Location + FVector(0, 0, 100.f));
		// The original NPCs mostly face the viewer (south-east); turn them that way.
		const FRotator Facing(0.f, GetActorRotation().Yaw + 90.f, 0.f);
		// Zone-specific rows are "<template>@<zone>"; skins are keyed by the plain template name.
		FString TemplateName = Spawn.Character.ToString();
		TemplateName.Split(TEXT("@"), &TemplateName, nullptr);
		FName Skin = CharacterSkins.FindRef(FName(*TemplateName));
		if (Skin.IsNone())
		{
			const TArray<FName>& Pool = Spawn.Role == EAstoniaNpcRole::Thief ? ThiefSkins
				: Spawn.Role == EAstoniaNpcRole::Merchant ? MerchantSkins : QuestGiverSkins;
			if (!Pool.IsEmpty()) Skin = Pool[GetTypeHash(TemplateName) % Pool.Num()];
		}
		if (AAstoniaNpc* Npc = GetWorld()->SpawnActor<AAstoniaNpc>(Location, Facing, Params))
		{
			Npc->Setup(Spawn.Character, Spawn.DisplayName, Spawn.Role, Spawn.bFemale, Skin);
		}
	}
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
	UMaterialInterface* Material = PlaceholderMaterial.LoadSynchronous();
	for (UHierarchicalInstancedStaticMeshComponent* Layer : Layers())
	{
		Layer->Modify();
		Layer->ClearInstances();
		if (Material) Layer->SetMaterial(0, Material);
	}
	TArray<UActorComponent*> OldLights = GetComponentsByTag(UPointLightComponent::StaticClass(), LightTag);
	for (UActorComponent* Old : OldLights)
	{
		RemoveInstanceComponent(Old);
		Old->DestroyComponent();
	}
	Items.Reset();
	Spawns.Reset();
	Shafts.Reset();

	const int32 N = Map.Size;
	auto Index = [N](int32 X, int32 Y) { return X + Y * N; };
	auto InMap = [N](int32 X, int32 Y) { return X >= 0 && Y >= 0 && X < N && Y < N; };

	// --- Item templates per tile (needed for walkability) ---
	TArray<const FAstoniaItemRow*> TileItem;
	TArray<FName> TileItemName;
	TileItem.SetNumZeroed(N * N);
	TileItemName.SetNum(N * N);
	int32 MissingItems = 0;
	for (int32 i = 0; i < N * N; i++)
	{
		if (Map.Tiles[i].Item.IsEmpty()) continue;
		TileItem[i] = FindTemplate<FAstoniaItemRow>(ItemsDT, Map.Tiles[i].Item, Zone, TileItemName[i]);
		MissingItems += TileItem[i] == nullptr;
	}

	// --- Walkable sections (4-connected), like the player could walk in the 2D game ---
	TArray<bool> Walkable;
	Walkable.SetNumZeroed(N * N);
	for (int32 i = 0; i < N * N; i++)
	{
		const FAstoniaMapTile& T = Map.Tiles[i];
		const FAstoniaItemRow* It = TileItem[i];
		const bool bItemBlocks = It && It->HasFlag(TEXT("IF_MOVEBLOCK")) && !It->HasFlag(TEXT("IF_DOOR"));
		Walkable[i] = T.GroundSprite != 0 && !EnumHasAnyFlags(T.Flags, EAstoniaMapFlag::MoveBlock) && !bItemBlocks;
	}
	TArray<int32> Section;
	Section.Init(INDEX_NONE, N * N);
	int32 NumSections = 0;
	{
		TArray<int32> Queue;
		for (int32 Start = 0; Start < N * N; Start++)
		{
			if (!Walkable[Start] || Section[Start] != INDEX_NONE) continue;
			Section[Start] = NumSections;
			Queue.Reset();
			Queue.Add(Start);
			for (int32 Q = 0; Q < Queue.Num(); Q++)
			{
				const int32 C = Queue[Q];
				const int32 CX = C % N, CY = C / N;
				for (const FIntPoint D : {FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1)})
				{
					const int32 NX = CX + D.X, NY = CY + D.Y;
					if (InMap(NX, NY) && Walkable[Index(NX, NY)] && Section[Index(NX, NY)] == INDEX_NONE)
					{
						Section[Index(NX, NY)] = NumSections;
						Queue.Add(Index(NX, NY));
					}
				}
			}
			NumSections++;
		}
	}

	// --- Levels: 0 = surface, 1.. = Placements ---
	TArray<FLevel> Levels;
	Levels.AddDefaulted();
	TArray<int32> SectionLevel;
	SectionLevel.Init(0, NumSections);
	for (int32 P = 0; P < Placements.Num(); P++)
	{
		const FAstoniaSectionPlacement& Pl = Placements[P];
		const int32 ParentLevel = Pl.Parent < 0 ? 0 : Pl.Parent + 1;
		const int32 EntrySection = InMap(Pl.EntryTile.X, Pl.EntryTile.Y) ? Section[Index(Pl.EntryTile.X, Pl.EntryTile.Y)] : INDEX_NONE;
		if (ParentLevel > P || EntrySection == INDEX_NONE)
		{
			UE_LOG(LogAstoniaCore, Error, TEXT("%s: placement %d (%s): entry %d,%d is not walkable or parent comes later"),
				*GetName(), P, *Pl.Label, Pl.EntryTile.X, Pl.EntryTile.Y);
			Levels.AddDefaulted();
			continue;
		}
		FLevel& L = Levels.AddDefaulted_GetRef();
		L.Offset = Levels[ParentLevel].Offset + Pl.ColumnTile - Pl.EntryTile;
		L.Z = -Pl.Depth;
		L.Section = EntrySection;
		SectionLevel[EntrySection] = P + 1;
	}

	// --- Every tile goes to the level(s) of the nearest section(s): walls follow the rooms they enclose ---
	TArray<TArray<int32, TInlineAllocator<2>>> TileLevels;
	TileLevels.SetNum(N * N);
	{
		TArray<int32> Dist;
		Dist.Init(MAX_int32, N * N);
		TArray<int32> Queue;
		for (int32 i = 0; i < N * N; i++)
		{
			if (Section[i] != INDEX_NONE)
			{
				Dist[i] = 0;
				TileLevels[i].Add(SectionLevel[Section[i]]);
				Queue.Add(i);
			}
		}
		for (int32 Q = 0; Q < Queue.Num(); Q++)
		{
			const int32 C = Queue[Q];
			const int32 CX = C % N, CY = C / N;
			for (int32 DY = -1; DY <= 1; DY++)
			{
				for (int32 DX = -1; DX <= 1; DX++)
				{
					const int32 NX = CX + DX, NY = CY + DY;
					if (!InMap(NX, NY)) continue;
					const int32 Ni = Index(NX, NY);
					if (Dist[Ni] == MAX_int32)
					{
						Dist[Ni] = Dist[C] + 1;
						Queue.Add(Ni);
					}
					if (Dist[Ni] == Dist[C] + 1)
					{
						for (int32 L : TileLevels[C]) TileLevels[Ni].AddUnique(L);
					}
				}
			}
		}
		for (int32 i = 0; i < N * N; i++)
		{
			if (TileLevels[i].IsEmpty()) TileLevels[i].Add(0);
		}
	}

	auto LevelHasTile = [&](int32 Level, int32 X, int32 Y) { return InMap(X, Y) && TileLevels[Index(X, Y)].Contains(Level); };

	// --- Shafts: no floor where a platform sits, hollow casing where a shaft passes another level ---
	TSet<TPair<int32, int32>> NoFloor;   // (level, original tile index)
	TSet<TPair<int32, int32>> Casing;    // (level, original tile index)
	TSet<TPair<int32, int32>> NoCeiling; // (level, original tile index)
	for (int32 P = 0; P < Placements.Num(); P++)
	{
		const FAstoniaSectionPlacement& Pl = Placements[P];
		const FLevel& L = Levels[P + 1];
		if (L.Section == INDEX_NONE) continue;
		const int32 ParentLevel = Pl.Parent < 0 ? 0 : Pl.Parent + 1;
		const FLevel& Top = Levels[ParentLevel];
		const FIntPoint Column = Top.Offset + Pl.ColumnTile;  // world tile of the shaft

		// The upper floor opens for the platform; the lower floor stays (the platform rests on it).
		NoFloor.Add({ParentLevel, Index(Pl.ColumnTile.X, Pl.ColumnTile.Y)});
		NoCeiling.Add({P + 1, Index(Pl.EntryTile.X, Pl.EntryTile.Y)});
		for (int32 Other = 0; Other < Levels.Num(); Other++)
		{
			const FLevel& O = Levels[Other];
			if (Other == ParentLevel || Other == P + 1 || !(O.Z < Top.Z && O.Z > L.Z)) continue;
			const FIntPoint Orig = Column - O.Offset;
			if (LevelHasTile(Other, Orig.X, Orig.Y)) Casing.Add({Other, Index(Orig.X, Orig.Y)});
		}

		FAstoniaShaft& S = Shafts.AddDefaulted_GetRef();
		S.Label = Pl.Label;
		S.Top = TileToLocal(Column.X, Column.Y) + FVector(0, 0, Top.Z);
		S.Drop = Top.Z - L.Z - 20.f;  // the platform stops on top of the lower floor
		S.bSecretDoor = Pl.bSecretDoor;
		const FAstoniaMapTile& ColumnTile = Map.At(Pl.ColumnTile.X, Pl.ColumnTile.Y);
		S.WallColor = Pick(WallPalette, ColumnTile.ForegroundSprite / 1000);
	}

	auto WallHeightAt = [&](int32 Level, int32 X, int32 Y)
	{
		float H = WallHeight;
		if (Level != 0) return H;
		for (const FAstoniaHeightRegion& R : HeightRegions)
		{
			if (X >= R.Min.X && X <= R.Max.X && Y >= R.Min.Y && Y <= R.Max.Y) H = R.WallHeight;
		}
		return H;
	};

	// Replaced[i]: 0 = built here, 1 = a structure replaces it but the ground stays, 2 = replaced with the ground.
	TArray<uint8> Replaced;
	Replaced.SetNumZeroed(N * N);
	for (const FAstoniaReplacedRegion& R : ReplacedRegions)
	{
		for (int32 Y = FMath::Max(R.Min.Y, 0); Y <= FMath::Min(R.Max.Y, N - 1); Y++)
		{
			for (int32 X = FMath::Max(R.Min.X, 0); X <= FMath::Min(R.Max.X, N - 1); X++)
			{
				Replaced[Index(X, Y)] = FMath::Max<uint8>(Replaced[Index(X, Y)], R.bKeepGround ? 1 : 2);
			}
		}
	}

	// Engine basic shapes are 100 cm across and centred on their pivot.
	const float S = AstoniaTileSize / 100.f;
	int32 MissingChars = 0;

	for (int32 Y = 0; Y < N; Y++)
	{
		for (int32 X = 0; X < N; X++)
		{
			const int32 I = Index(X, Y);
			const FAstoniaMapTile& Tile = Map.Tiles[I];
			const bool bBlocked = EnumHasAnyFlags(Tile.Flags, EAstoniaMapFlag::MoveBlock);
			const bool bAnything = Tile.GroundSprite || Tile.ForegroundSprite || !Tile.Item.IsEmpty() || !Tile.Character.IsEmpty() || bBlocked;
			if (!bAnything) continue;

			for (int32 Level : TileLevels[I])
			{
				const FLevel& L = Levels[Level];
				const FVector P = TileToLocal(X + L.Offset.X, Y + L.Offset.Y) + FVector(0, 0, L.Z);
				const bool bCasing = Casing.Contains({Level, I});
				const float WallH = WallHeightAt(Level, X, Y);
				const uint8 Replace = Level == 0 ? Replaced[I] : 0;

				if (Replace)
				{
					// A hand-built structure stands here: keep only the content lists (and the ground if asked).
					if (Replace == 1)
					{
						AddInstance(Ground, FTransform(FQuat::Identity, P - FVector(0, 0, 10.f), FVector(S, S, 0.2f)),
							Pick(GroundPalette, Tile.GroundSprite / 100));
					}
				}
				else if (!NoFloor.Contains({Level, I}) && !bCasing)
				{
					FLinearColor C = Pick(GroundPalette, Tile.GroundSprite / 100);
					if (EnumHasAnyFlags(Tile.Flags, EAstoniaMapFlag::Underwater)) C = Water;
					else if (Level != 0) C = EnumHasAnyFlags(Tile.Flags, EAstoniaMapFlag::Indoors) ? IndoorFloor : CaveFloor;
					else if (EnumHasAnyFlags(Tile.Flags, EAstoniaMapFlag::Indoors)) C = IndoorFloor;
					AddInstance(Ground, FTransform(FQuat::Identity, P - FVector(0, 0, 10.f), FVector(S, S, 0.2f)), C);
				}

				if (Replace)
				{
					// The structure brings its own walls and props.
				}
				else if (bCasing)
				{
					// Four thin walls around the shaft, open in the middle for the platform.
					const float Thick = 0.1f;
					const float Off = AstoniaTileSize * 0.5f - Thick * 50.f;
					for (const FVector2D D : {FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1)})
					{
						const FVector Scale = D.X != 0 ? FVector(Thick, S, WallH / 100.f) : FVector(S, Thick, WallH / 100.f);
						AddInstance(Walls, FTransform(FQuat::Identity, P + FVector(D.X * Off, D.Y * Off, WallH * 0.5f), Scale), ShaftWall);
					}
				}
				else if (bBlocked)
				{
					AddInstance(Walls, FTransform(FQuat::Identity, P + FVector(0, 0, WallH * 0.5f), FVector(S, S, WallH / 100.f)),
						Pick(WallPalette, Tile.ForegroundSprite / 1000));
				}
				else if (Tile.ForegroundSprite)
				{
					AddInstance(Props, FTransform(FQuat::Identity, P + FVector(0, 0, 50.f), FVector(0.8f, 0.8f, 1.f)),
						Pick(WallPalette, Tile.ForegroundSprite / 1000));
				}

				if (bBuildRoofs && Level == 0 && !Replace && !bBlocked && EnumHasAnyFlags(Tile.Flags, EAstoniaMapFlag::Indoors) && !NoFloor.Contains({Level, I}))
				{
					AddInstance(Roofs, FTransform(FQuat::Identity, P + FVector(0, 0, WallH + 10.f), FVector(S, S, 0.2f)), Roof);
				}
				// Underground levels are closed in, so daylight stays out and only torches light them.
				if (Level != 0 && !bCasing && !NoCeiling.Contains({Level, I}))
				{
					AddInstance(Roofs, FTransform(FQuat::Identity, P + FVector(0, 0, WallH + 10.f), FVector(S, S, 0.2f)), CaveFloor);
				}

				if (!Tile.Item.IsEmpty() && Level == TileLevels[I][0])
				{
					const FAstoniaItemRow* Row = TileItem[I];
					const EAstoniaZoneItemKind Kind = Classify(Row);
					Items.Add({TileItemName[I], Kind, FIntPoint(X, Y), P});

					// Holes and ladders inside this zone are replaced by lift shafts.
					const bool bLocalTeleport = Kind == EAstoniaZoneItemKind::Teleport && Row && TeleportArea(*Row) == 0;
					if (!bLocalTeleport && !bCasing && !Replace)
					{
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
								if (InMap(X + D.X, Y + D.Y) && Walkable[Index(X + D.X, Y + D.Y)])
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
				}

				if (!Tile.Character.IsEmpty() && Level == TileLevels[I][0])
				{
					FName RowName;
					const FAstoniaCharacterRow* Row = FindTemplate<FAstoniaCharacterRow>(CharsDT, Tile.Character, Zone, RowName);
					MissingChars += Row == nullptr;
					const bool bHostile = Row && !Row->HasFlag(TEXT("CF_NOATTACK")) && !Row->HasFlag(TEXT("CF_IMMORTAL"));
					EAstoniaNpcRole NpcRole = EAstoniaNpcRole::Monster;
					if (ThiefTemplates.ContainsByPredicate([&](const FString& Prefix) { return Tile.Character.StartsWith(Prefix); }))
					{
						NpcRole = EAstoniaNpcRole::Thief;
					}
					else if (Row && !bHostile && Row->HasFlag(TEXT("CF_HASNAME")))
					{
						NpcRole = Row->Driver == 72 ? EAstoniaNpcRole::Merchant : EAstoniaNpcRole::QuestGiver;  // 72: trader driver
					}
					FAstoniaZoneSpawn& Spawn = Spawns.Add_GetRef({RowName, FIntPoint(X, Y), P, bHostile, NpcRole});
					Spawn.DisplayName = Row ? Row->DisplayName : Tile.Character;
					Spawn.bFemale = Row && Row->HasFlag(TEXT("CF_FEMALE"));
					if (!CharacterRoles.Contains(NpcRole))
					{
						AddInstance(NpcMarkers, FTransform(FQuat::Identity, P + FVector(0, 0, 95.f), FVector(0.8f, 0.8f, 1.9f)),
							bHostile ? Hostile : Friendly);
					}
				}
			}
		}
	}

	for (UHierarchicalInstancedStaticMeshComponent* Layer : Layers())
	{
		Layer->MarkRenderStateDirty();
	}

	UE_LOG(LogAstoniaCore, Display, TEXT("%s: built from %s: %d sections, %d levels, %d shafts, %d ground, %d walls, %d roofs, %d props, %d items (%d unknown), %d NPCs (%d unknown)"),
		*GetName(), *MapFile, NumSections, Levels.Num(), Shafts.Num(), Ground->GetInstanceCount(), Walls->GetInstanceCount(),
		Roofs->GetInstanceCount(), Props->GetInstanceCount(), Items.Num(), MissingItems, Spawns.Num(), MissingChars);
}
#endif
