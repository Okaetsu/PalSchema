#include "Loader/Spawner/PalSpawnGroupListInfo.h"
#include "nlohmann/json.hpp"
#include "Utility/JsonHelpers.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS {
    void PalSpawnGroupListInfo::AddPalListInfo(const nlohmann::json& value)
    {
        if (!value.is_object())
        {
            throw std::runtime_error("PalList item was not an object.");
        }

        Palworld::PalListInfo listInfo;
        if (PS::JsonHelpers::FieldExists(value, "NPCID"))
        {
            PS::JsonHelpers::ParseFName(value, "NPCID", listInfo.NPCID);
        }

        if (PS::JsonHelpers::FieldExists(value, "PalId"))
        {
            PS::JsonHelpers::ParseFName(value, "PalId", listInfo.PalId);
        }

        if (listInfo.NPCID == NAME_None && listInfo.PalId == NAME_None)
        {
            throw std::runtime_error("PalList item must contain either NPCID or PalId.");
        }

        if (PS::JsonHelpers::FieldExists(value, "Level"))
        {
            PS::JsonHelpers::ParseInteger(value, "Level", listInfo.Level);
        }

        if (PS::JsonHelpers::FieldExists(value, "Level_Max"))
        {
            PS::JsonHelpers::ParseInteger(value, "Level_Max", listInfo.LevelMax);
        }

        if (PS::JsonHelpers::FieldExists(value, "Num"))
        {
            PS::JsonHelpers::ParseInteger(value, "Num", listInfo.Num);
        }

        if (PS::JsonHelpers::FieldExists(value, "Num_Max"))
        {
            PS::JsonHelpers::ParseInteger(value, "Num_Max", listInfo.NumMax);
        }

        PalList.push_back(listInfo);
    }
}