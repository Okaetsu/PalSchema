#pragma once

#include "Unreal/AActor.hpp"
#include "SDK/Structs/PalSpawnerGroup.h"

namespace Palworld {
    class APalSpawnerStandard : public RC::Unreal::AActor {
    public:
        void AddSpawnerGroup(const PalSpawnerGroup& spawnerGroup);

        void SetSpawnerName(const RC::Unreal::FName& spawnerName);

        void SetSpawnerType(const RC::Unreal::uint8& spawnerType);
    };
}