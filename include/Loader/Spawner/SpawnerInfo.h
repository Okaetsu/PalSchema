#pragma once

#include "Unreal/NameTypes.hpp"
#include "Unreal/UnrealCoreStructs.hpp"
#include "Unreal/Rotator.hpp"
#include "SDK/Structs/Guid.h"
#include "Loader/Spawner/PalSpawnGroupListInfo.h"
#include "nlohmann/json_fwd.hpp"

namespace RC::Unreal {
    class UWorld;
}

namespace UECustom {
    class UWorldPartitionRuntimeLevelStreamingCell;
}

namespace PS {
    class AMonoNPCSpawner;

    enum class SpawnerType : RC::Unreal::uint8 {
        Undefined,
        Sheet,
        MonoNPC
    };

    struct SpawnerInfo {
        bool bExistsInWorld = false;
        RC::Unreal::AActor* SpawnerActor = nullptr;
        UECustom::UWorldPartitionRuntimeLevelStreamingCell* Cell = nullptr;
        RC::StringType ModName{};
        UECustom::FGuid Guid;

        // Generic

        SpawnerType Type = SpawnerType::Undefined;
        RC::Unreal::FVector Location;
        RC::Unreal::FRotator Rotation;

        // MonoNPCSpawner

        int Level = 1;
        RC::Unreal::FName NPCID = RC::Unreal::NAME_None;
        RC::Unreal::FName OtomoName = RC::Unreal::NAME_None;

        // Sheet

        RC::Unreal::FName SpawnerName = RC::Unreal::NAME_None;
        RC::Unreal::uint8 SpawnerType = 0;
        std::vector<PS::PalSpawnGroupListInfo> SpawnGroupList;
        bool bHasMapIcon = false;

        void Unload();

        void AddSpawnGroupList(const nlohmann::json& value);

        RC::StringType ToString();
    private:
        RC::StringType CachedString = TEXT("");

        RC::Unreal::uint8 GetOnlyTimeFromString(const std::string& str);
    };
}