#pragma once

#include "Unreal/AActor.hpp"

namespace Palworld {
    class AMonoNPCSpawner : public RC::Unreal::AActor {
    public:
        int& GetLevel();

        RC::Unreal::FName& GetHumanName();

        RC::Unreal::FName& GetCharaName();

        void Spawn();
    };
}