#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/TSoftClassPtr.h"

namespace Palworld {
    struct FPalNPCTalkFlowClassDataRow : public RC::Unreal::FTableRowBase
    {
        FPalNPCTalkFlowClassDataRow(const RC::StringType& Path) : NPCTalkFlowClass(UECustom::TSoftClassPtr<RC::Unreal::UClass>(UECustom::FSoftObjectPath(Path)))
        {
        }
        UECustom::TSoftClassPtr<RC::Unreal::UClass> NPCTalkFlowClass;
    };
}