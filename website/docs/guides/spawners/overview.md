---
sidebar_position: 1
---

# Overview

Starting with 0.5.0, you are now able to create new spawns for pals and NPCs which function the exact same as how the game would spawn pals, bosses, npcs, etc. Editing is not currently available and you should use the `blueprints` method of doing it. See the example [BossMammorestSpawnReplacer](https://github.com/Okaetsu/PalSchema/blob/main/assets/examples/BossMammorestSpawnReplacer/blueprints/mammorest_spawn_replacer.jsonc) mod for editing spawns.

Features:
- Auto-reload is **supported** for spawner mods, [video](https://www.youtube.com/watch?v=jK6SnkGTL50) of it in action.
- When adding bosses, an icon is automatically added to the map without having to edit `DT_BossSpawnerLoactionData` as long as the `SpawnerType` is set to `FieldBoss`.

We will be going through the structure of a spawner json and the required folder name for it.

## Folder Name

The required name for the folder so it can be seen by PalSchema is `spawners`, meaning you want something like this `MyModName/spawners/spawn.json`.

## JSON Structure

**IMPORTANT**: You need to start your spawner JSON as an array with square brackets [ ] rather than curly brackets \{ \}.

Currently the following fields are available:

### Generic

These fields are available to both `MonoNPC` and `Sheet`.

- `Type`: Can be either `MonoNPC` or `Sheet`.
- `Location`: Location of the spawned character(s).
- `Rotation`: Rotation of the spawned character(s).

### MonoNPC
These fields are only available when `Type` is set to `MonoNPC`.
- `NPCID`: ID of your NPC, e.g. PalDealer. Only available when `Type` is set to `MonoNPC`.
- `OtomoId`: ID of the character that is summoned when the NPC is attacked. Only available when `Type` is set to `MonoNPC`.
- `Level`: Level of the NPC specified by `NPCID`.

### Sheet
These fields are only available when `Type` is set to `Sheet`.
- `SpawnerName`: Name of the spawner.
- `SpawnerType`: Type specified in the `EPalSpawnerPlacementType` enum as a string, e.g. `FieldBoss` for a boss.
- `SpawnGroupList`: Array of `SpawnGroup` objects.  
  - `SpawnGroup`:
    - `Weight`: If there are multiple SpawnGroups, this controls how likely it is that this specific group will be selected over the others. Should be a whole integer from 1 to 100, decimals are not accepted.
    - `OnlyTime`: Controls if the SpawnGroup should only spawn at a certain time of day. Accepted string values are `Day` and `Night`. You may also omit this field entirely if you want the group to spawn regardless of the time of day.
    - `PalList`: Array of `PalSpawnerOneTribeInfo` objects.
      - `PalSpawnerOneTribeInfo`:
        - `PalId`: String Id of the pal to spawn, e.g. PinkCat.
        - `Level`: Minimum level of the spawned character.
        - `Level_Max`: Maximum level of the spawned character.
        - `Num`: Minimum number of characters to spawn at the same time.
        - `Num_Max`: Maximum number of characters to spawn at the same time.