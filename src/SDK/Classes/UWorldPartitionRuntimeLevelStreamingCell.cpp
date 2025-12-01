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
}