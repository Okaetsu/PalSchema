#pragma once

#include "SDK/Structs/PalListInfo.h"

namespace Palworld {
    struct PalSpawnerGroup {
        RC::Unreal::FName OriginalRowName = RC::Unreal::NAME_None;
        RC::Unreal::FName OriginalSpawnerName = RC::Unreal::NAME_None;
        int Weight = 10;
        RC::Unreal::uint8 OnlyTime = 0;
        RC::Unreal::uint8 OnlyWeather = 0;
        std::vector<PalListInfo> PalList;
    };
}