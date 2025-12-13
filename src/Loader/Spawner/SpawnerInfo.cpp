#include "Loader/Spawner/SpawnerInfo.h"
#include "SDK/Classes/AMonoNPCSpawner.h"
#include "nlohmann/json.hpp"
#include "Utility/Logging.h"
#include "Utility/JsonHelpers.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS {
    void SpawnerInfo::Unload()
    {
        if (SpawnerActor && !SpawnerActor->IsUnreachable())
        {
            PS::Log<LogLevel::Verbose>(STR("[{}] Preparing to destroy spawner actor.\n"), ModName);
            SpawnerActor->K2_DestroyActor();
            SpawnerActor = nullptr;
            PS::Log<LogLevel::Verbose>(STR("[{}] Finished destroying spawner actor.\n"), ModName);
        }

        if (Cell)
        {
            Cell = nullptr;
        }

        bExistsInWorld = false;
    }

    void SpawnerInfo::AddSpawnGroupList(const nlohmann::json& value)
    {
        if (!value.is_object())
        {
            throw std::runtime_error("SpawnGroupList item was not an object.");
        }

        PS::JsonHelpers::ValidateFieldExists(value, "Weight");
        PS::JsonHelpers::ValidateFieldExists(value, "PalList");
        auto& palList = value.at("PalList");

        PalSpawnGroupListInfo spawnGroupListInfo;
        PS::JsonHelpers::ParseInteger(value, "Weight", spawnGroupListInfo.Weight);

        if (PS::JsonHelpers::FieldExists(value, "OnlyTime"))
        {
            std::string onlyTime = "Undefined";
            PS::JsonHelpers::ParseString(value, "OnlyTime", onlyTime);
            spawnGroupListInfo.OnlyTime = GetOnlyTimeFromString(onlyTime);
        }
        
        if (!palList.is_array())
        {
            throw std::runtime_error("PalList must be an array of objects.");
        }

        for (auto& palListItem : palList)
        {
            spawnGroupListInfo.AddPalListInfo(palListItem);
        }

        SpawnGroupList.push_back(spawnGroupListInfo);
    }

    RC::StringType SpawnerInfo::ToString()
    {
        if (CachedString != TEXT(""))
        {
            return CachedString;    
        }

        RC::StringType location = std::format(TEXT("X: {:.3f}, Y: {:.3f}, Z: {:.3f}"), Location.GetX(), Location.GetY(), Location.GetZ());

        if (Type == SpawnerType::Sheet)
        {
            CachedString = std::format(TEXT("(Sheet @ [{}] with {} group{})"),
                location,
                SpawnGroupList.size(),
                SpawnGroupList.size() > 1 ? TEXT("s") : TEXT(""));
            return CachedString;
        }

        RC::StringType npcId = NPCID.ToString();
        CachedString = std::format(TEXT("({} @ [{}])"), npcId, location);;
        return CachedString;
    }

    RC::Unreal::uint8 SpawnerInfo::GetOnlyTimeFromString(const std::string& str)
    {
        if (str == "Day")
        {
            return 1U;
        }

        if (str == "Night")
        {
            return 2U;
        }

        return 0U;
    }
}