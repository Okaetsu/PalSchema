#pragma once

#include "Unreal/UObject.hpp"

namespace RC::Unreal {
    class UWorld;
}

namespace UECustom {
    class UWorldPartition : public RC::Unreal::UObject {
    public:
        RC::Unreal::UWorld* GetWorld();
    };
}