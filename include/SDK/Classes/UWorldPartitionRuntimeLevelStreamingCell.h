#pragma once

#include "Unreal/UObject.hpp"
#include "SDK/Structs/FBox.h"

namespace UECustom {
    class ULevelStreaming;

    class UWorldPartitionRuntimeLevelStreamingCell : public RC::Unreal::UObject {
    public:
        FBox& GetContentBounds();

        ULevelStreaming* GetLevelStreaming();

        bool& GetIsHLOD();

        RC::Unreal::FVector& GetPosition();

        float& GetExtent();

        int& GetLevel();
    };
}