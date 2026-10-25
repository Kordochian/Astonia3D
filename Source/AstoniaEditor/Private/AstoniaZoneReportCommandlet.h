#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AstoniaZoneReportCommandlet.generated.h"

/**
 * Reads every .itm, .chr and .map file of the original zones and writes a report of what is in
 * them (counts, drivers, flags, keys, parse errors). It is the checklist for the import phase.
 *
 *   UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=AstoniaZoneReport [-Zones=<dir>] [-Out=<file>]
 */
UCLASS()
class UAstoniaZoneReportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAstoniaZoneReportCommandlet();
	virtual int32 Main(const FString& Params) override;
};
