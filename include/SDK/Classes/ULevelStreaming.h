#pragma once

#include "Unreal/UObject.hpp"
#include "Unreal/FWeakObjectPtr.hpp"

namespace UECustom {
    class UWorldPartition;

    class ULevelStreaming : public RC::Unreal::UObject {
    public:
        UWorldPartition* GetOuterWorldPartition();
    };
}