#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AstoniaImportTemplatesCommandlet.generated.h"

/**
 * Imports every .itm and .chr template of the original zones into two DataTables:
 * /Game/Astonia/Data/DT_Items and /Game/Astonia/Data/DT_Characters. Re-running replaces the rows.
 *
 * Row names are the template names. A name defined in more than one zone keeps the first
 * definition (generic, then zones in folder order) under its plain name; the others become
 * "<name>@<zone>".
 *
 *   UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=AstoniaImportTemplates [-Zones=<dir>]
 */
UCLASS()
class UAstoniaImportTemplatesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAstoniaImportTemplatesCommandlet();
	virtual int32 Main(const FString& Params) override;
};
