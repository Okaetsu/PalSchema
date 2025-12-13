#include "SDK/Classes/UWorldPartitionRuntimeLevelStreamingCell.h"
#include "Helpers/Casting.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    FBox& UECustom::UWorldPartitionRuntimeLevelStreamingCell::GetContentBounds()
    {
        return *Helper::Casting::ptr_cast<FBox*>(this, 0x48);
    }

    ULevelStreaming* UWorldPartitionRuntimeLevelStreamingCell::GetLevelStreaming()
    {
        return *Helper::Casting::ptr_cast<ULevelStreaming**>(this, 0x138);
    }

    bool& UWorldPartitionRuntimeLevelStreamingCell::GetIsHLOD()
    {
        return *Helper::Casting::ptr_cast<bool*>(this, 0xB5);
    }

    RC::Unreal::FVector& UWorldPartitionRuntimeLevelStreamingCell::GetPosition()
    {
        return *Helper::Casting::ptr_cast<RC::Unreal::FVector*>(this, 0xE8);
    }

    float& UWorldPartitionRuntimeLevelStreamingCell::GetExtent()
    {
        return *Helper::Casting::ptr_cast<float*>(this, 0x100);
    }

    int& UWorldPartitionRuntimeLevelStreamingCell::GetLevel()
    {
        return *Helper::Casting::ptr_cast<int*>(this, 0x104);
    }
}