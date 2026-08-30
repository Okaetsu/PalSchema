#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/SoftObjectPtr.hpp"

namespace Palworld {
    struct FPalCharacterIconDataRow : public RC::Unreal::FTableRowBase
    {
        FPalCharacterIconDataRow(const RC::StringType& Path) 
            : Icon(RC::Unreal::TSoftObjectPtr<RC::Unreal::UObject>(RC::Unreal::FSoftObjectPath(RC::Unreal::FString(Path))))
        {
        }

        RC::Unreal::TSoftObjectPtr<RC::Unreal::UObject> Icon;
    };
}