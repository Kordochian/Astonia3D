#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AstoniaDataRows.generated.h"

struct FAstoniaTemplate;

/** One mod_index/mod_value (or req_index/req_value) pair of an item. */
USTRUCT(BlueprintType)
struct ASTONIACORE_API FAstoniaStatModifier
{
	GENERATED_BODY()

	/** V_* name, e.g. V_LIGHT, V_STR. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Stat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Value = 0;

	/** req_index: a minimum the wearer needs instead of a bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	bool bRequirement = false;
};

/** An item template from a .itm file (create.c process_item_template). */
USTRUCT(BlueprintType)
struct ASTONIACORE_API FAstoniaItemRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Template name used by maps and other templates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Template;

	/** Zone folder the template came from ("generic" or an area number). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Zone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString Description;

	/** Original sprite id. Only used to pick a placeholder or new mesh, never to load original art. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Sprite = 0;

	/** IF_* flags, e.g. IF_TAKE, IF_MOVEBLOCK, IF_DOOR. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	TArray<FName> Flags;

	/** Behaviour id (item drivers in the server); 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Driver = 0;

	/** Driver data as the original hex string; each driver decodes its own layout. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString DriverArg;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	TArray<FAstoniaStatModifier> Modifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Quality = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Complexity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 MinLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 MaxLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 NeedsClass = 0;

	bool HasFlag(FName Flag) const { return Flags.Contains(Flag); }
};

/** A random drop (rprob= followed by ritem=). Probability is out of 10000. */
USTRUCT(BlueprintType)
struct ASTONIACORE_API FAstoniaRandomDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Probability = 0;
};

/** A character template from a .chr file (create.c process_char_template). */
USTRUCT(BlueprintType)
struct ASTONIACORE_API FAstoniaCharacterRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Template;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Zone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString Description;

	/** Original character model id: picks the placeholder or new mesh/mob type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Sprite = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Sound = 0;

	/** CF_* flags, e.g. CF_IMMORTAL, CF_NOATTACK, CF_RESPAWN. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	TArray<FName> Flags;

	/** Base stats and skills by V_* name (V_HP, V_STR, V_SWORD, V_FIRE...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	TMap<FName, int32> Values;

	/** Worn items by WN_* slot (WN_RHAND, WN_BODY...) -> item template. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	TMap<FName, FName> Worn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	TArray<FName> Inventory;

	/** Spell item templates the character starts with. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	TArray<FName> Spells;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	TArray<FAstoniaRandomDrop> RandomDrops;

	/** AI/behaviour id (character drivers in the server). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Driver = 0;

	/** Driver text, all arg= lines joined as the server does. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	FString DriverArg;

	/** NPC faction; characters of one group do not fight each other. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Group = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Class = 0;

	/** Seconds until the NPC respawns (server default 60). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 RespawnSeconds = 60;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 GoldProb = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 GoldBase = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 GoldRandom = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 SpecialProb = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 SpecialStrength = 0;

	/** Level the server balances this NPC's stats to; 0 = use the values as written. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 Balanced = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 HPBoost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astonia")
	int32 ManaBoost = 0;

	bool HasFlag(FName Flag) const { return Flags.Contains(Flag); }
	int32 GetValue(FName Stat) const { const int32* V = Values.Find(Stat); return V ? *V : 0; }
};

namespace AstoniaDataRows
{
	/** Convert parsed templates. Unknown keys are reported in OutWarnings and otherwise ignored. */
	ASTONIACORE_API FAstoniaItemRow MakeItemRow(const FAstoniaTemplate& T, FName Zone, TArray<FString>& OutWarnings);
	ASTONIACORE_API FAstoniaCharacterRow MakeCharacterRow(const FAstoniaTemplate& T, FName Zone, TArray<FString>& OutWarnings);
}
