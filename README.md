# AstoniaUE5

A 3D open-world rebuild of Astonia 3.5 in Unreal Engine 5.8.3. Game rules and world data come from the
community 3.5 server in `F:\After_Astonia\source_codes\astonia_community_server35-main`. No original
sprites or sounds are used: everything visual is a placeholder until new art exists.

Based on the Astonia 3 engine by Daniel Brockhaus - https://brockhaus.org (required credit, also drawn
on the in-game HUD).

## Modules

| Module | What goes there |
|---|---|
| `AstoniaCore` | Ported rules and data formats, no actors. Now: readers for `.map`, `.itm`, `.chr` (`AstoniaTextFormat.h`), following `get_text()`/`process_map()` in the server's `create.c`. |
| `AstoniaGame` | The game: `AAstoniaGameMode`, `AAstoniaCharacter` (free third-person camera, placeholder cylinder body), `AAstoniaPlayerController` (WASD/mouse/gamepad bound in code), `AAstoniaHUD`. |
| `AstoniaEditor` | Editor tools and importers. Now: the `AstoniaZoneReport` commandlet. |

Conventions: 1 Astonia tile = 200 cm (`AstoniaTileSize`), an area is 256x256 tiles, the server ran 24 ticks/s.

## Build and run

1. Right-click `AstoniaUE5.uproject` > Generate Visual Studio project files, or build from a shell:
   `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" AstoniaUE5Editor Win64 Development -Project="F:\After_Astonia\UE_port\AstoniaUE5\AstoniaUE5.uproject"`
2. Open `AstoniaUE5.uproject`. It starts in `/Game/Maps/L_World` (World Partition open world). Press Play.

Controls: WASD move, mouse look, Space jump (gamepad: sticks and A).

## Tools

- Zone report (checks every zone file parses and lists drivers, flags and keys still to port):
  `UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=AstoniaZoneReport [-Zones=<dir>] [-Out=<file>]`
  Writes `Saved\AstoniaZoneReport.txt`.
- `Scripts\create_world.py` recreates `L_World` from the engine's OpenWorld template if it is missing:
  `UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>\Scripts\create_world.py"`

## Plan

See `F:\After_Astonia\UE5-REBUILD-PLAN.md`. Phase 0 (this skeleton) is done; next is Phase 1 (import the
templates into DataTables) and Phase 2 (build Cameron in the open world).
