#include "AstoniaDataRows.h"
#include "AstoniaCore.h"
#include "AstoniaTextFormat.h"

namespace
{
	bool Is(const FString& Key, const TCHAR* Name) { return Key.Equals(Name, ESearchCase::IgnoreCase); }
	int32 Int(const FString& Value) { return FCString::Atoi(*Value); }
	FName Upper(const FString& Value) { return FName(*Value.ToUpper()); }
}

FAstoniaItemRow AstoniaDataRows::MakeItemRow(const FAstoniaTemplate& T, FName Zone, TArray<FString>& OutWarnings)
{
	FAstoniaItemRow Row;
	Row.Template = FName(*T.Name);
	Row.Zone = Zone;

	// mod_index/req_index name the stat, the following mod_value/req_value closes the pair (cmod++).
	FAstoniaStatModifier Pending;
	bool bPending = false;

	for (const FAstoniaKeyValue& E : T.Entries)
	{
		const FString& K = E.Key;
		const FString& V = E.Value;
		if (Is(K, TEXT("name"))) Row.DisplayName = V;
		else if (Is(K, TEXT("description"))) Row.Description = V;
		else if (Is(K, TEXT("quality"))) Row.Quality = Int(V);
		else if (Is(K, TEXT("complexity"))) Row.Complexity = Int(V);
		else if (Is(K, TEXT("value"))) Row.Value = Int(V);
		else if (Is(K, TEXT("sprite"))) Row.Sprite = Int(V);
		else if (Is(K, TEXT("driver"))) Row.Driver = Int(V);
		else if (Is(K, TEXT("min_level"))) Row.MinLevel = Int(V);
		else if (Is(K, TEXT("max_level"))) Row.MaxLevel = Int(V);
		else if (Is(K, TEXT("needs_class"))) Row.NeedsClass = Int(V);
		else if (Is(K, TEXT("id"))) Row.ID = V;
		else if (Is(K, TEXT("arg"))) Row.DriverArg = V;
		else if (Is(K, TEXT("flag"))) Row.Flags.AddUnique(Upper(V));
		else if (Is(K, TEXT("mod_index")) || Is(K, TEXT("req_index")))
		{
			Pending.Stat = Upper(V);
			Pending.bRequirement = Is(K, TEXT("req_index"));
			bPending = true;
		}
		else if (Is(K, TEXT("mod_value")) || Is(K, TEXT("req_value")))
		{
			if (!bPending) OutWarnings.Add(FString::Printf(TEXT("item %s: %s without an index"), *T.Name, *K));
			Pending.Value = Int(V);
			Row.Modifiers.Add(Pending);
			Pending = FAstoniaStatModifier();
			bPending = false;
		}
		else OutWarnings.Add(FString::Printf(TEXT("item %s: unknown key %s"), *T.Name, *K));
	}
	return Row;
}

FAstoniaCharacterRow AstoniaDataRows::MakeCharacterRow(const FAstoniaTemplate& T, FName Zone, TArray<FString>& OutWarnings)
{
	FAstoniaCharacterRow Row;
	Row.Template = FName(*T.Name);
	Row.Zone = Zone;
	int32 PendingProb = 0;

	for (const FAstoniaKeyValue& E : T.Entries)
	{
		const FString& K = E.Key;
		const FString& V = E.Value;
		if (Is(K, TEXT("name"))) Row.DisplayName = V;
		else if (Is(K, TEXT("description"))) Row.Description = V;
		else if (Is(K, TEXT("sprite"))) Row.Sprite = Int(V);
		else if (Is(K, TEXT("sound"))) Row.Sound = Int(V);
		else if (Is(K, TEXT("gold"))) Row.Gold = Int(V);
		else if (Is(K, TEXT("driver"))) Row.Driver = Int(V);
		else if (Is(K, TEXT("group"))) Row.Group = Int(V);
		else if (Is(K, TEXT("class"))) Row.Class = Int(V);
		else if (Is(K, TEXT("respawn"))) Row.RespawnSeconds = Int(V);
		else if (Is(K, TEXT("special_prob"))) Row.SpecialProb = Int(V);
		else if (Is(K, TEXT("special_strength"))) Row.SpecialStrength = Int(V);
		else if (Is(K, TEXT("gold_prob"))) Row.GoldProb = Int(V);
		else if (Is(K, TEXT("gold_base"))) Row.GoldBase = Int(V);
		else if (Is(K, TEXT("gold_random"))) Row.GoldRandom = Int(V);
		else if (Is(K, TEXT("balanced"))) Row.Balanced = Int(V);
		else if (Is(K, TEXT("hpboost"))) Row.HPBoost = Int(V);
		else if (Is(K, TEXT("manaboost"))) Row.ManaBoost = Int(V);
		else if (Is(K, TEXT("arg"))) Row.DriverArg += V;
		else if (Is(K, TEXT("item"))) Row.Inventory.Add(FName(*V));
		else if (Is(K, TEXT("spell"))) Row.Spells.Add(FName(*V));
		else if (Is(K, TEXT("rprob"))) PendingProb = Int(V);
		else if (Is(K, TEXT("ritem")))
		{
			if (PendingProb <= 0) OutWarnings.Add(FString::Printf(TEXT("character %s: ritem %s without rprob"), *T.Name, *V));
			Row.RandomDrops.Add({FName(*V), PendingProb});
		}
		else if (Is(K, TEXT("flag"))) Row.Flags.AddUnique(Upper(V));
		else if (K.StartsWith(TEXT("V_"), ESearchCase::IgnoreCase)) Row.Values.Add(Upper(K), Int(V));
		else if (K.StartsWith(TEXT("WN_"), ESearchCase::IgnoreCase)) Row.Worn.Add(Upper(K), FName(*V));
		else OutWarnings.Add(FString::Printf(TEXT("character %s: unknown key %s"), *T.Name, *K));
	}
	return Row;
}
