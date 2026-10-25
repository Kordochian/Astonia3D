#pragma once

#include "CoreMinimal.h"
#include "Misc/Paths.h"

namespace AstoniaEditorPaths
{
	/** Default zone data: the community 3.5 server checked out next to the UE project. */
	inline FString DefaultZonesDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../source_codes/astonia_community_server35-main/zones"));
	}
}
