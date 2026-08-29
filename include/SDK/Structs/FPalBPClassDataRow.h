#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/SoftObjectPtr.hpp"

namespace Palworld {
    struct FPalBPClassDataRow : public RC::Unreal::FTableRowBase
    {
        FPalBPClassDataRow(const RC::StringType& Path) 
            : BPClass(RC::Unreal::TSoftObjectPtr<RC::Unreal::UObject>(RC::Unreal::FSoftObjectPath(RC::Unreal::FString(Path))))
        {
        }
        RC::Unreal::TSoftObjectPtr<RC::Unreal::UObject> BPClass;
    };
}