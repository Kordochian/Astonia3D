#include "AstoniaZoneReportCommandlet.h"
#include "AstoniaTextFormat.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogAstoniaZoneReport, Log, All);

namespace
{
	/** Default: the community 3.5 server checked out next to the UE project. */
	FString DefaultZonesDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../source_codes/astonia_community_server35-main/zones"));
	}

	void Count(TMap<FString, int32>& Map, const FString& Key) { Map.FindOrAdd(Key)++; }

	void AppendSorted(FString& Report, const TCHAR* Title, TMap<FString, int32>& Map)
	{
		Map.ValueSort([](int32 A, int32 B) { return A > B; });
		Report += FString::Printf(TEXT("\n== %s (%d distinct)\n"), Title, Map.Num());
		for (const TPair<FString, int32>& P : Map)
		{
			Report += FString::Printf(TEXT("%8d  %s\n"), P.Value, *P.Key);
		}
	}
}

UAstoniaZoneReportCommandlet::UAstoniaZoneReportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UAstoniaZoneReportCommandlet::Main(const FString& Params)
{
	FString ZonesDir = DefaultZonesDir();
	FParse::Value(*Params, TEXT("Zones="), ZonesDir);
	FString OutFile = FPaths::ProjectSavedDir() / TEXT("AstoniaZoneReport.txt");
	FParse::Value(*Params, TEXT("Out="), OutFile);

	if (!IFileManager::Get().DirectoryExists(*ZonesDir))
	{
		UE_LOG(LogAstoniaZoneReport, Error, TEXT("Zones folder not found: %s"), *ZonesDir);
		return 1;
	}

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *ZonesDir, TEXT("*.*"), true, false);
	Files.Sort();

	int32 NumItems = 0, NumChars = 0, NumMaps = 0, NumErrors = 0;
	TSet<FString> ItemNames, CharNames;
	TMap<FString, int32> ItemDrivers, CharDrivers, ItemFlags, CharFlags, ItemKeys, CharKeys, MapFlags;
	TMap<FString, int32> PlacedItems, PlacedChars;
	FString Errors, MapLines;

	for (const FString& File : Files)
	{
		const FString Ext = FPaths::GetExtension(File).ToLower();
		if (Ext != TEXT("itm") && Ext != TEXT("chr") && Ext != TEXT("map")) continue;

		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *File))
		{
			Errors += FString::Printf(TEXT("%s: cannot read\n"), *File);
			NumErrors++;
			continue;
		}

		FString Rel = File;
		FPaths::MakePathRelativeTo(Rel, *(ZonesDir / TEXT("")));
		FString Error;

		if (Ext == TEXT("map"))
		{
			FAstoniaMap Map;
			if (!AstoniaTextFormat::ParseMap(Text, Map, Error))
			{
				Errors += FString::Printf(TEXT("%s: %s\n"), *Rel, *Error);
				NumErrors++;
				continue;
			}
			NumMaps++;
			int32 Ground = 0, Fore = 0, Items = 0, Chars = 0, Blocked = 0;
			for (const FAstoniaMapTile& T : Map.Tiles)
			{
				Ground += T.GroundSprite != 0;
				Fore += T.ForegroundSprite != 0;
				Blocked += EnumHasAnyFlags(T.Flags, EAstoniaMapFlag::MoveBlock);
				if (!T.Item.IsEmpty()) { Items++; Count(PlacedItems, T.Item); }
				if (!T.Character.IsEmpty()) { Chars++; Count(PlacedChars, T.Character); }
				for (int32 Bit = 0; Bit < 20; Bit++)
				{
					if ((uint32(T.Flags) >> Bit) & 1) Count(MapFlags, FString::Printf(TEXT("bit %d"), Bit));
				}
			}
			MapLines += FString::Printf(TEXT("%-40s ground %6d  fore %6d  blocked %6d  items %5d  chars %4d\n"),
				*Rel, Ground, Fore, Blocked, Items, Chars);
			continue;
		}

		TArray<FAstoniaTemplate> Templates;
		if (!AstoniaTextFormat::ParseTemplates(Text, Templates, Error))
		{
			Errors += FString::Printf(TEXT("%s: %s\n"), *Rel, *Error);
			NumErrors++;
		}

		const bool bItem = Ext == TEXT("itm");
		for (const FAstoniaTemplate& T : Templates)
		{
			(bItem ? ItemNames : CharNames).Add(T.Name.ToLower());
			(bItem ? NumItems : NumChars)++;
			if (const FString* Driver = T.Find(TEXT("driver"))) Count(bItem ? ItemDrivers : CharDrivers, *Driver);
			for (const FAstoniaKeyValue& E : T.Entries)
			{
				if (E.Key.Equals(TEXT("flag"), ESearchCase::IgnoreCase)) Count(bItem ? ItemFlags : CharFlags, E.Value.ToUpper());
				else Count(bItem ? ItemKeys : CharKeys, E.Key.ToLower());
			}
		}
	}

	// Templates the maps place but no file defines.
	TMap<FString, int32> MissingItems, MissingChars;
	for (const TPair<FString, int32>& P : PlacedItems) if (!ItemNames.Contains(P.Key.ToLower())) MissingItems.Add(P);
	for (const TPair<FString, int32>& P : PlacedChars) if (!CharNames.Contains(P.Key.ToLower())) MissingChars.Add(P);

	FString Report = FString::Printf(TEXT("Astonia zone report\nZones: %s\n\nItem templates: %d\nCharacter templates: %d\nMaps: %d\nFiles with parse errors: %d\n"),
		*ZonesDir, NumItems, NumChars, NumMaps, NumErrors);
	Report += TEXT("\n== Parse errors\n") + (Errors.IsEmpty() ? FString(TEXT("none\n")) : Errors);
	Report += TEXT("\n== Maps\n") + MapLines;
	AppendSorted(Report, TEXT("Map flag bits (MF_tab order)"), MapFlags);
	AppendSorted(Report, TEXT("Item drivers"), ItemDrivers);
	AppendSorted(Report, TEXT("Character drivers"), CharDrivers);
	AppendSorted(Report, TEXT("Item flags"), ItemFlags);
	AppendSorted(Report, TEXT("Character flags"), CharFlags);
	AppendSorted(Report, TEXT("Item keys"), ItemKeys);
	AppendSorted(Report, TEXT("Character keys"), CharKeys);
	AppendSorted(Report, TEXT("Placed items with no template"), MissingItems);
	AppendSorted(Report, TEXT("Placed characters with no template"), MissingChars);

	FFileHelper::SaveStringToFile(Report, *OutFile);
	UE_LOG(LogAstoniaZoneReport, Display, TEXT("%d item templates, %d character templates, %d maps, %d files with errors. Report: %s"),
		NumItems, NumChars, NumMaps, NumErrors, *OutFile);
	return 0;
}
