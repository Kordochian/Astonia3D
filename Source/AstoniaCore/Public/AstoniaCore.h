#pragma once

#include "CoreMinimal.h"

ASTONIACORE_API DECLARE_LOG_CATEGORY_EXTERN(LogAstoniaCore, Log, All);

/** Size of one original area (server.h MAXMAP). */
constexpr int32 AstoniaAreaSize = 256;

/** Server ticks per second (server.h TICKS). Rules that count in ticks convert with this. */
constexpr int32 AstoniaTicksPerSecond = 24;

/** Original tile edge in Unreal units (cm). One Astonia tile holds one character. */
constexpr float AstoniaTileSize = 200.f;
