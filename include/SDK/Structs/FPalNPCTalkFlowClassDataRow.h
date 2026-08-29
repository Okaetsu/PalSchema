#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/SoftObjectPtr.hpp"

namespace Palworld {
    struct FPalNPCTalkFlowClassDataRow : public RC::Unreal::FTableRowBase
    {
        FPalNPCTalkFlowClassDataRow(const RC::StringType& Path) 
            : NPCTalkFlowClass(RC::Unreal::TSoftObjectPtr<RC::Unreal::UObject>(RC::Unreal::FSoftObjectPath(RC::Unreal::FString(Path))))
        {
        }
        RC::Unreal::TSoftObjectPtr<RC::Unreal::UObject> NPCTalkFlowClass;
    };
}