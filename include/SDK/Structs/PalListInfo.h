#pragma once

#include "Unreal/NameTypes.hpp"

namespace Palworld {
    // Do not pass to UE internals, see FPalSpawnerOneTribeInfo and FPalSpawnerGroupInfo instead in /Reflected
    struct PalListInfo {
        RC::Unreal::FName PalId = RC::Unreal::NAME_None;
        RC::Unreal::FName NPCID = RC::Unreal::NAME_None;
        int Level = 1;
        int LevelMax = 1;
        int Num = 1;
        int NumMax = 1;
    };
}