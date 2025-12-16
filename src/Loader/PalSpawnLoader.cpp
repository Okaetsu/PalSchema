#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Unreal/UEnum.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/Transform.hpp"
#include "Unreal/World.hpp"
#include "SDK/Classes/AMonoNPCSpawner.h"
#include "SDK/Classes/APalSpawnerStandard.h"
#include "SDK/Classes/UWorldPartition.h"
#include "SDK/Classes/UWorldPartitionRuntimeLevelStreamingCell.h"
#include "SDK/Classes/ULevelStreaming.h"
#include "SDK/Classes/KismetGuidLibrary.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "SDK/Classes/Custom/DataTable/TableSerializer.h"
#include "Utility/Logging.h"
#include "Utility/EnumHelpers.h"
#include "Utility/JsonHelpers.h"
#include "Loader/PalSpawnLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace Palworld {
    PalSpawnLoader::PalSpawnLoader() : PalModLoaderBase("spawns") {}

    PalSpawnLoader::~PalSpawnLoader() {}

    void PalSpawnLoader::Initialize()
    {
        m_bossSpawnerLocationData = UECustom::UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
            STR("/Game/Pal/DataTable/UI/DT_BossSpawnerLoactionData.DT_BossSpawnerLoactionData"));
    }

    void PalSpawnLoader::Load(const std::filesystem::path::string_type& modName, const nlohmann::json& data)
    {
        if (!data.is_array())
        {
            throw std::runtime_error("Spawn JSON must start as an array rather than as an object.");
        }

        // The json file itself starts as an array, rather than as an object
        for (auto& value : data)
        {
            RegisterSpawn(modName, value);
        }
    }

    void PalSpawnLoader::Reload(const std::filesystem::path::string_type& modName, const nlohmann::json& data)
    {
        UnloadMod(modName);
        Load(modName, data);

        for (auto loadedCell : m_loadedCells)
        {
            ProcessCellSpawners(loadedCell);
        }
    }

    void PalSpawnLoader::OnWorldCleanup(RC::Unreal::UWorld* World)
    {
        static auto NAME_MainWorld5 = FName(STR("PL_MainWorld5"), FNAME_Add);
        
        if (NAME_MainWorld5 != World->GetNamePrivate())
        {
            return;
        }

        // Main world is unloading (returning to title).
        // We want to reset all the containers so that our spawners can be spawned again when we re-enter the world.
        for (auto& spawnInfo : m_spawns)
        {
            spawnInfo.Cell = nullptr;
            spawnInfo.SpawnerActor = nullptr;
            spawnInfo.bExistsInWorld = false;
        }

        m_loadedCells.Empty();

        PS::Log<LogLevel::Verbose>(STR("Session ending, spawners have been cleaned up.\n"));
    }

    void PalSpawnLoader::OnCellLoaded(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell)
    {
        // This isn't perfect, but it should hopefully do the job for now until a better solution is found.
        // Seems like cells can overlap each other (?) which means the spawner code can get called multiple times.

        if (cell->GetExtent() > 51200.0)
        {
            // Skip massive cells. Hopefully it doesn't cause issues with some locations not working for spawners.
            return;
        }

        if (cell->GetIsHLOD())
        {
            // Skip HLOD. From what I noticed, if a spawner is created inside a HLOD, it more than likely will not despawn ever which is not ideal.
            return;
        }

        if (cell->GetLevel() == 0)
        {
            // Skip L0 grids, all the other spawners seem to be on L1 or above.
            return;
        }

        m_loadedCells.Add(cell);
        ProcessCellSpawners(cell);
    }

    void PalSpawnLoader::OnCellUnloaded(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell)
    {
        DestroySpawnersInCell(cell);
        m_loadedCells.Remove(cell);
    }

    void PalSpawnLoader::RegisterSpawn(const std::filesystem::path::string_type& modName, const nlohmann::json& value)
    {
        PS::JsonHelpers::ValidateFieldExists(value, "Type");
        PS::JsonHelpers::ValidateFieldExists(value, "Location");
        PS::JsonHelpers::ValidateFieldExists(value, "Rotation");

        PS::SpawnerInfo spawnerInfo{};

        auto Guid = UECustom::UKismetGuidLibrary::NewGuid();
        spawnerInfo.Guid = Guid;
        spawnerInfo.ModName = modName;

        std::string type;
        PS::JsonHelpers::ParseString(value, "Type", type);

        if (type == "MonoNPC")
        {
            spawnerInfo.Type = PS::SpawnerType::MonoNPC;
        }
        else if (type == "Sheet")
        {
            spawnerInfo.Type = PS::SpawnerType::Sheet;
        }
        else
        {
            throw std::runtime_error("Type must be 'MonoNPC' or 'Sheet'.");
        }

        // Location is required for all spawner types
        PS::JsonHelpers::ParseVector(value, "Location", spawnerInfo.Location);

        // Rotation is required for all spawner types
        PS::JsonHelpers::ParseRotator(value, "Rotation", spawnerInfo.Rotation);

        if (spawnerInfo.Type == PS::SpawnerType::Sheet)
        {
            RegisterSheet(modName, spawnerInfo, value);
        }
        else if (spawnerInfo.Type == PS::SpawnerType::MonoNPC)
        {
            RegisterMonoNPC(modName, spawnerInfo, value);
        }

        m_spawns.push_back(spawnerInfo);

        Output::send<LogLevel::Normal>(STR("Added new spawn: {}\n"), spawnerInfo.ToString());
    }

    void PalSpawnLoader::RegisterSheet(const std::filesystem::path::string_type& modName, PS::SpawnerInfo& spawnerInfo, const nlohmann::json& value)
    {
        PS::JsonHelpers::ValidateFieldExists(value, "SpawnGroupList");
        auto& spawnGroupList = value.at("SpawnGroupList");

        if (!spawnGroupList.is_array())
        {
            throw std::runtime_error("SpawnGroupList must be an array of objects.");
        }

        for (auto& spawnGroupListItem : spawnGroupList)
        {
            spawnerInfo.AddSpawnGroupList(spawnGroupListItem);
        }

        if (PS::JsonHelpers::FieldExists(value, "SpawnerName"))
        {
            std::string spawnerName;
            PS::JsonHelpers::ParseString(value, "SpawnerName", spawnerName);

            auto spawnerNameWide = RC::to_generic_string(spawnerName);
            spawnerNameWide = std::format(TEXT("{}_{}"), spawnerInfo.ModName, spawnerNameWide);

            spawnerInfo.SpawnerName = FName(spawnerNameWide, FNAME_Add);
        }

        if (PS::JsonHelpers::FieldExists(value, "SpawnerType"))
        {
            static auto ENUM_EPalSpawnedCharacterType = UECustom::UObjectGlobals::StaticFindObject<UEnum*>(nullptr, nullptr,
                STR("/Script/Pal.EPalSpawnedCharacterType"));

            std::string spawnerType;
            PS::JsonHelpers::ParseString(value, "SpawnerType", spawnerType);
            spawnerInfo.SpawnerType = PS::EnumHelpers::GetEnumValueByName(ENUM_EPalSpawnedCharacterType, spawnerType);

            if (spawnerType.ends_with("FieldBoss"))
            {
                AddBossSpawnLocationToMap(spawnerInfo);
            }
        }
    }

    void PalSpawnLoader::RegisterMonoNPC(const std::filesystem::path::string_type& modName, PS::SpawnerInfo& spawnerInfo, const nlohmann::json& value)
    {
        PS::JsonHelpers::ValidateFieldExists(value, "NPCID");
        PS::JsonHelpers::ValidateFieldExists(value, "Level");

        PS::JsonHelpers::ParseFName(value, "NPCID", spawnerInfo.NPCID);
        PS::JsonHelpers::ParseInteger(value, "Level", spawnerInfo.Level);

        if (spawnerInfo.Type == PS::SpawnerType::MonoNPC && spawnerInfo.NPCID == NAME_None)
        {
            throw std::runtime_error("NPCID can not be 'None' when type is set to 'MonoNPC'");
        }

        // This will be the Pal that is summoned by the NPC when it is attacked. Optional field
        if (PS::JsonHelpers::FieldExists(value, "OtomoId"))
        {
            PS::JsonHelpers::ParseFName(value, "OtomoId", spawnerInfo.OtomoName);
        }
    }

    void PalSpawnLoader::ProcessCellSpawners(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell)
    {
        auto& Box = cell->GetContentBounds();
        for (auto& spawn : m_spawns)
        {
            if (spawn.bExistsInWorld) continue;

            if (Box.IsInside(spawn.Location))
            {
                CreateSpawner(cell, spawn);
            }
        }
    }

    void PalSpawnLoader::CreateSpawner(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell, PS::SpawnerInfo& spawnerInfo)
    {
        switch (spawnerInfo.Type)
        {
        case PS::SpawnerType::MonoNPC:
            SpawnMonoNPC(cell, spawnerInfo);
            break;
        case PS::SpawnerType::Sheet:
            SpawnSheet(cell, spawnerInfo);
            break;
        }
    }

    void PalSpawnLoader::SpawnMonoNPC(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell, PS::SpawnerInfo& spawnerInfo)
    {
        static auto bpClass = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Game/Pal/Blueprint/Spawner/BP_MonoNPCSpawner.BP_MonoNPCSpawner_C"));
        if (!bpClass)
        {
            PS::Log<LogLevel::Error>(STR("Unable to get class BP_MonoNPCSpawner, failed to spawn {}\n"), spawnerInfo.NPCID.ToString());
            return;
        }

        auto world = cell->GetWorld();
        if (!world)
        {
            PS::Log<LogLevel::Error>(STR("Unable to get world from cell, failed to spawn {}\n"), spawnerInfo.NPCID.ToString());
            return;
        }

        auto transform = FTransform(spawnerInfo.Rotation, spawnerInfo.Location, { 1.0, 1.0, 1.0 });
        auto spawnedActor = world->SpawnActor(bpClass, &transform);
        auto monoSpawner = static_cast<AMonoNPCSpawner*>(spawnedActor);
        monoSpawner->GetHumanName() = spawnerInfo.NPCID;
        monoSpawner->GetCharaName() = spawnerInfo.NPCID;
        monoSpawner->GetLevel() = spawnerInfo.Level;
        monoSpawner->GetOtomoName() = spawnerInfo.OtomoName;

        spawnerInfo.bExistsInWorld = true;
        spawnerInfo.Cell = cell;
        spawnerInfo.SpawnerActor = monoSpawner;

        PS::Log<LogLevel::Verbose>(STR("Spawned {} in {}\n"), spawnerInfo.ToString(), cell->GetName());
    }

    void PalSpawnLoader::SpawnSheet(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell, PS::SpawnerInfo& spawnerInfo)
    {
        static auto sheetBPClass = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Game/Pal/Blueprint/Spawner/BP_PalSpawner_Standard.BP_PalSpawner_Standard_C"));
        if (!sheetBPClass)
        {
            PS::Log<LogLevel::Error>(STR("Unable to get class BP_PalSpawner_Standard, failed to spawn sheet\n"));
            return;
        }

        auto world = cell->GetWorld();
        if (!world)
        {
            PS::Log<LogLevel::Error>(STR("Unable to get world from cell, failed to spawn sheet\n"));
            return;
        }

        auto transform = FTransform(spawnerInfo.Rotation, spawnerInfo.Location, { 1.0, 1.0, 1.0 });
        auto spawnedActor = world->SpawnActor(sheetBPClass, &transform);
        auto palSheet = static_cast<APalSpawnerStandard*>(spawnedActor);

        PalSpawnerGroup spawnerGroup;
        for (auto& spawnGroupListItem : spawnerInfo.SpawnGroupList)
        {
            spawnerGroup.Weight = spawnGroupListItem.Weight;
            spawnerGroup.OnlyTime = spawnGroupListItem.OnlyTime;
            spawnerGroup.OnlyWeather = spawnGroupListItem.OnlyWeather;

            for (auto& palListInfo : spawnGroupListItem.PalList)
            {
                spawnerGroup.PalList.push_back(palListInfo);
            }
        }

        palSheet->SetSpawnerName(spawnerInfo.SpawnerName);
        palSheet->SetSpawnerType(spawnerInfo.SpawnerType);
        palSheet->AddSpawnerGroup(spawnerGroup);

        spawnerInfo.bExistsInWorld = true;
        spawnerInfo.Cell = cell;
        spawnerInfo.SpawnerActor = palSheet;

        PS::Log<LogLevel::Verbose>(STR("Spawned {} in {}\n"), spawnerInfo.ToString(), cell->GetName());
    }

    void PalSpawnLoader::AddBossSpawnLocationToMap(PS::SpawnerInfo& spawnerInfo)
    {
        if (!m_bossSpawnerLocationData) {
            PS::Log<LogLevel::Error>(STR("Failed to add boss icon to map, DT_BossSpawnerLoactionData was not initialized.\n"));
            return;
        }

        if (spawnerInfo.SpawnGroupList.size() == 0)
        {
            PS::Log<LogLevel::Error>(STR("Failed to add boss icon to map, SpawnGroupList has no entries.\n"));
            return;
        }

        auto& firstGroup = spawnerInfo.SpawnGroupList.at(0);
        if (firstGroup.PalList.size() == 0)
        {
            PS::Log<LogLevel::Error>(STR("Failed to add boss icon to map, PalList has no entries.\n"));
            return;
        }

        auto& firstPal = firstGroup.PalList.at(0);

        auto characterId = firstPal.PalId;
        if (characterId == NAME_None)
        {
            characterId = firstPal.NPCID;
        }

        TableSerializer serializer(m_bossSpawnerLocationData);

        auto newRow = serializer.Add(spawnerInfo.SpawnerName);
        newRow->SetValue<FName>(STR("SpawnerID"), spawnerInfo.SpawnerName);
        newRow->SetValue<FName>(STR("CharacterID"), characterId);
        newRow->SetValue<FVector>(STR("Location"), spawnerInfo.Location);
        newRow->SetValue<int>(STR("Level"), firstPal.Level);

        spawnerInfo.bHasMapIcon = true;
    }

    void PalSpawnLoader::RemoveBossSpawnLocationFromMap(PS::SpawnerInfo& spawnerInfo)
    {
        if (!m_bossSpawnerLocationData) {
            PS::Log<LogLevel::Error>(STR("Failed to remove boss icon from map, DT_BossSpawnerLoactionData was not initialized.\n"));
            return;
        }

        // This doesn't seem to actually help.
        // WBP_Map_Base saves boss icons permanently to a variable called BossIcons.
        // The logic goes something like this: 
        //  (WBP_Map_Base):"Setup Boss Icon" -> 
        //  (WBP_Map_Base):"Add Boss Icon" -> 
        //  (WBP_Map_Base).(WBP_Map_Body): Add Icon By Location
        // Ideally we'd want to clear the icons manually from WBP_Map_Body and then run "Setup Boss Icon" in WBP_Map_Base again to refresh them.
        m_bossSpawnerLocationData->RemoveRow(spawnerInfo.SpawnerName);
    }

    void PalSpawnLoader::DestroySpawnersInCell(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell)
    {
        for (auto& spawn : m_spawns)
        {
            if (spawn.Cell == cell)
            {
                spawn.Unload();
                PS::Log<LogLevel::Verbose>(STR("Unloaded spawn in Cell {}\n"), cell->GetName());
            }
        }
    }

    void PalSpawnLoader::UnloadMod(const std::filesystem::path::string_type& modName)
    {
        std::erase_if(m_spawns, [&](PS::SpawnerInfo& spawn) {
            if (spawn.ModName == modName)
            {
                spawn.Unload();

                if (spawn.bHasMapIcon)
                {
                    RemoveBossSpawnLocationFromMap(spawn);
                }

                return true;
            }

            return false;
        });
    }
}