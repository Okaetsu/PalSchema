#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/TSoftClassPtr.h"
#include "SDK/Classes/Texture2D.h"

namespace Palworld {
    struct FPalBPClassDataRow : public RC::Unreal::FTableRowBase
    {
        FPalBPClassDataRow(const RC::StringType& Path) : BPClass(UECustom::TSoftClassPtr<RC::Unreal::UClass>(UECustom::FSoftObjectPath(Path)))
        {
        }
        UECustom::TSoftClassPtr<RC::Unreal::UClass> BPClass;
    };
}