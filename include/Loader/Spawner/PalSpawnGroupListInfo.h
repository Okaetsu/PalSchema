#pragma once

#include <vector>
#include "SDK/Structs/PalListInfo.h"
#include "nlohmann/json_fwd.hpp"

namespace PS {
    struct PalSpawnGroupListInfo {
        int Weight = 10;
        RC::Unreal::uint8 OnlyTime = 0;
        RC::Unreal::uint8 OnlyWeather = 0;
        std::vector<Palworld::PalListInfo> PalList;

        void AddPalListInfo(const nlohmann::json& value);
    };
}