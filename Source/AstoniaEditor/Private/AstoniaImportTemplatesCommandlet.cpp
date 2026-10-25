#include "AstoniaImportTemplatesCommandlet.h"
#include "AstoniaDataRows.h"
#include "AstoniaEditorPaths.h"
#include "AstoniaTextFormat.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogAstoniaImport, Log, All);

namespace
{
	UDataTable* MakeTable(const FString& PackagePath, UScriptStruct* RowStruct)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		UPackage* Package = CreatePackage(*PackagePath);
		Package->FullyLoad();

		UDataTable* Table = FindObject<UDataTable>(Package, *AssetName);
		if (!Table)
		{
			Table = NewObject<UDataTable>(Package, *AssetName, RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Table);
		}
		Table->EmptyTable();
		Table->RowStruct = RowStruct;
		return Table;
	}

	bool SaveTable(UDataTable* Table)
	{
		UPackage* Package = Table->GetOutermost();
		Package->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Table, *File, Args);
	}

	/** "generic" first, then numbered zone folders in numeric order. */
	FName ZoneOf(const FString& File, const FString& ZonesDir)
	{
		FString Rel = File;
		FPaths::MakePathRelativeTo(Rel, *(ZonesDir / TEXT("")));
		FString Zone;
		Rel.Split(TEXT("/"), &Zone, nullptr);
		return FName(*Zone);
	}

	template <typename RowType>
	void AddRow(UDataTable* Table, RowType& Row, int32& Duplicates)
	{
		FName Key = Row.Template;
		if (Table->FindRowUnchecked(Key))
		{
			Key = FName(*FString::Printf(TEXT("%s@%s"), *Row.Template.ToString(), *Row.Zone.ToString()));
			Duplicates++;
			for (int32 Suffix = 2; Table->FindRowUnchecked(Key); Suffix++)
			{
				Key = FName(*FString::Printf(TEXT("%s@%s_%d"), *Row.Template.ToString(), *Row.Zone.ToString(), Suffix));
			}
		}
		Table->AddRow(Key, Row);
	}
}

UAstoniaImportTemplatesCommandlet::UAstoniaImportTemplatesCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UAstoniaImportTemplatesCommandlet::Main(const FString& Params)
{
	FString ZonesDir = AstoniaEditorPaths::DefaultZonesDir();
	FParse::Value(*Params, TEXT("Zones="), ZonesDir);
	if (!IFileManager::Get().DirectoryExists(*ZonesDir))
	{
		UE_LOG(LogAstoniaImport, Error, TEXT("Zones folder not found: %s"), *ZonesDir);
		return 1;
	}

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *ZonesDir, TEXT("*.*"), true, false);
	Files.Sort([&ZonesDir](const FString& A, const FString& B)
	{
		const FString ZA = ZoneOf(A, ZonesDir).ToString(), ZB = ZoneOf(B, ZonesDir).ToString();
		if (ZA != ZB)
		{
			if (ZA == TEXT("generic")) return true;
			if (ZB == TEXT("generic")) return false;
			const int32 NA = FCString::Atoi(*ZA), NB = FCString::Atoi(*ZB);
			if (NA != NB) return NA < NB;
			return ZA < ZB;
		}
		return A < B;
	});

	UDataTable* Items = MakeTable(TEXT("/Game/Astonia/Data/DT_Items"), FAstoniaItemRow::StaticStruct());
	UDataTable* Chars = MakeTable(TEXT("/Game/Astonia/Data/DT_Characters"), FAstoniaCharacterRow::StaticStruct());

	int32 ItemDuplicates = 0, CharDuplicates = 0, Errors = 0;
	TArray<FString> Warnings;

	for (const FString& File : Files)
	{
		const FString Ext = FPaths::GetExtension(File).ToLower();
		const bool bItem = Ext == TEXT("itm");
		if (!bItem && Ext != TEXT("chr")) continue;

		FString Text, Error;
		TArray<FAstoniaTemplate> Templates;
		if (!FFileHelper::LoadFileToString(Text, *File) || !AstoniaTextFormat::ParseTemplates(Text, Templates, Error))
		{
			UE_LOG(LogAstoniaImport, Error, TEXT("%s: %s"), *File, *Error);
			Errors++;
			continue;
		}

		const FName Zone = ZoneOf(File, ZonesDir);
		for (const FAstoniaTemplate& T : Templates)
		{
			if (bItem)
			{
				FAstoniaItemRow Row = AstoniaDataRows::MakeItemRow(T, Zone, Warnings);
				AddRow(Items, Row, ItemDuplicates);
			}
			else
			{
				FAstoniaCharacterRow Row = AstoniaDataRows::MakeCharacterRow(T, Zone, Warnings);
				AddRow(Chars, Row, CharDuplicates);
			}
		}
	}

	for (const FString& W : Warnings) UE_LOG(LogAstoniaImport, Warning, TEXT("%s"), *W);

	const bool bSaved = SaveTable(Items) && SaveTable(Chars);
	UE_LOG(LogAstoniaImport, Display, TEXT("DT_Items: %d rows (%d names defined in more than one zone). DT_Characters: %d rows (%d). %d warnings, %d file errors.%s"),
		Items->GetRowMap().Num(), ItemDuplicates, Chars->GetRowMap().Num(), CharDuplicates, Warnings.Num(), Errors,
		bSaved ? TEXT("") : TEXT(" SAVE FAILED"));
	return (bSaved && Errors == 0) ? 0 : 1;
}
