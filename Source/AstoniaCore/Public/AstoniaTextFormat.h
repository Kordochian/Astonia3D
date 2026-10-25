#pragma once

#include "CoreMinimal.h"

/** One `key=value` line from an Astonia zone file. */
struct FAstoniaKeyValue
{
	FString Key;
	FString Value;
};

/** A `name: ... ;` block from a .itm or .chr file. Keys can repeat (flag=, WN_*=). */
struct ASTONIACORE_API FAstoniaTemplate
{
	FString Name;
	TArray<FAstoniaKeyValue> Entries;

	/** First value for Key (case-insensitive), or nullptr. */
	const FString* Find(const FString& Key) const;
	void FindAll(const FString& Key, TArray<FString>& Out) const;
	int32 GetInt(const FString& Key, int32 Default = 0) const;
};

/** Map tile flags, same bit order as MF_tab in the server's create.c. */
enum class EAstoniaMapFlag : uint32
{
	None = 0,
	MoveBlock = 1u << 0,
	SightBlock = 1u << 1,
	TMoveBlock = 1u << 2,
	TSightBlock = 1u << 3,
	Indoors = 1u << 4,
	RestArea = 1u << 5,
	Door = 1u << 6,
	SoundBlock = 1u << 7,
	TSoundBlock = 1u << 8,
	ShoutBlock = 1u << 9,
	Clan = 1u << 10,
	Arena = 1u << 11,
	Peace = 1u << 12,
	Neutral = 1u << 13,
	FireThru = 1u << 14,
	SlowDeath = 1u << 15,
	NoLight = 1u << 16,
	NoMagic = 1u << 17,
	Underwater = 1u << 18,
	NoRegen = 1u << 19,
};
ENUM_CLASS_FLAGS(EAstoniaMapFlag);

struct FAstoniaMapTile
{
	int32 GroundSprite = 0;
	int32 ForegroundSprite = 0;
	EAstoniaMapFlag Flags = EAstoniaMapFlag::None;
	FString Item;      // item template placed here, if any
	FString Character; // character template placed here, if any
};

/** A parsed .map file: Size x Size tiles, index = X + Y * Size. */
struct ASTONIACORE_API FAstoniaMap
{
	int32 Size = 0;
	TArray<FAstoniaMapTile> Tiles;

	const FAstoniaMapTile& At(int32 X, int32 Y) const { return Tiles[X + Y * Size]; }
};

/**
 * Readers for the plain-text zone formats. They follow get_text() and process_map() in the
 * server's create.c, so anything the server loads, these load the same way.
 */
namespace AstoniaTextFormat
{
	ASTONIACORE_API bool ParseTemplates(const FString& Text, TArray<FAstoniaTemplate>& Out, FString& OutError);
	ASTONIACORE_API bool ParseMap(const FString& Text, FAstoniaMap& Out, FString& OutError);

	/** Bit index of an MF_* name, or INDEX_NONE. */
	ASTONIACORE_API int32 LookupMapFlag(const FString& Name);
}
