# AstoniaUE5

A 3D open-world rebuild of Astonia 3.5 in Unreal Engine 5.8.3. Game rules and world data come from the
community 3.5 server in `F:\After_Astonia\source_codes\astonia_community_server35-main`. No original
sprites or sounds are used: everything visual is a placeholder until new art exists. The player body is
Epic's UE5 mannequin (`Content/Characters/Mannequins`, copied from the engine's Third Person template
resources; free to use in Unreal Engine projects under the UE EULA).

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

On first start a character creator opens (name, body, skin, eyes, hair, beard; saved in `Saved/SaveGames`).
Controls: WASD move, mouse look, Space jump, E use (lifts, secret doors). Gamepad: sticks, A, X.

## Tools

- Zone report (checks every zone file parses and lists drivers, flags and keys still to port):
  `UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=AstoniaZoneReport [-Zones=<dir>] [-Out=<file>]`
  Writes `Saved\AstoniaZoneReport.txt`.
- Template import (Phase 1): `UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=AstoniaImportTemplates [-Zones=<dir>]`
  rebuilds `/Game/Astonia/Data/DT_Items` (3,607 rows) and `DT_Characters` (938 rows) from the `.itm`/`.chr` files.
  Row types: `FAstoniaItemRow`, `FAstoniaCharacterRow` in `AstoniaCore/Public/AstoniaDataRows.h`. A template name
  used in more than one zone keeps the first one (generic, then zones by number); later ones are `<name>@<zone>`.
- `Scripts\build_zone.py` (M1) builds an original area into `L_World` as an `AAstoniaZoneActor` (default: Cameron,
  area 1, at the world origin, PlayerStart on the new-character tile 126,179). The actor stores everything it
  builds (instanced ground/walls/props/items/NPC markers, torch lights, item and spawn lists), so the game never
  reads the original files; select it and press "Rebuild From Map" after the map changes.
  Verticality: areas the 2D game reached through teleport holes are moved under their entrance
  (`Placements`, set per area in the script's `LAYOUTS`), joined by `AAstoniaLift` platforms and, where
  wanted, hidden behind an `AAstoniaSecretDoor`; `HeightRegions` raise walls in part of an area.
  `ReplacedRegions` leave tiles to a hand-built structure. `Scripts\analyze_links.py` lists an area's
  walkable sections and teleports to plan this.
- `Scripts\build_fortress.py` (run after `build_zone.py`) builds Cameron's fortress, the start area, as an
  `AAstoniaBuilding`: instanced placeholder kit pieces (`EAstoniaKit`: stone, roofs, wood, foliage, glow...)
  laid out from `cameron.map` (wall lines, gate, galleries, buttresses, trees, props, braziers, floors) plus
  towers, a gatehouse, stairs and a wall walk all the way round. Materials `M_Kit` / `MI_KitStone` /
  `MI_KitPlain` / `M_KitGlow` (stone courses and flagstones in world space, so new art can replace layers later).
- `Scripts\build_thieves_guild.py` (after `build_zone.py`) builds the Thieves' Guild: a guildhouse against the
  fortress's east wall and, 6 m under it, the guild's 30 rooms from `cameron.map` (floors, walls, open doors,
  torches, chests, armchairs, fixed exposure), reached by a stair. Shared helpers live in `Scripts\astonia_kit.py`.
- `Scripts\shots.ps1 name=X,Y,Z,Yaw ...` runs the game and saves a screenshot from each spot
  (`-AstoniaGoto` / `-AstoniaShot`, development builds only). `-AstoniaFly` hovers for aerial views and
  `-AstoniaRoute=X,Y;X,Y;...` walks waypoints and logs where it got to (checks stairs and doorways).
- `Scripts\create_world.py` recreates an empty World Partition `L_World` (sky, sun, fog, PlayerStart; no terrain) if it is missing:
  `UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>\Scripts\create_world.py"`

## Plan

See `F:\After_Astonia\mds\UE5-REBUILD-PLAN.md` and the handoff note `F:\After_Astonia\mds\HANDOFF-UE5.md`.
Milestone plan: `F:\After_Astonia\mds\MILESTONES-UE5.md`. M0 is done; M1 has started with Cameron, the starter
zone, in the new open world map.
