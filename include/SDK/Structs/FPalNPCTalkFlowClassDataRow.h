#pragma once

#include "SDK/Classes/UDataTable.h"
#include "SDK/Classes/TSoftClassPtr.h"
#include "SDK/Structs/FTableRowBase.h"

namespace Palworld {
    struct FPalNPCTalkFlowClassDataRow : public UECustom::FTableRowBase
    {
        FPalNPCTalkFlowClassDataRow(const RC::StringType& Path) : NPCTalkFlowClass(UECustom::TSoftClassPtr<RC::Unreal::UClass>(UECustom::FSoftObjectPath(Path)))
        {
        }
        UECustom::TSoftClassPtr<RC::Unreal::UClass> NPCTalkFlowClass;
    };
}