#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/TSoftObjectPtr.h"
#include "SDK/Classes/Texture2D.h"

namespace Palworld {
    struct FPalCharacterIconDataRow : public RC::Unreal::FTableRowBase
    {
        FPalCharacterIconDataRow(const RC::StringType& Path) : Icon(UECustom::TSoftObjectPtr<UECustom::UTexture2D>(UECustom::FSoftObjectPath(Path)))
        {
        }

        UECustom::TSoftObjectPtr<UECustom::UTexture2D> Icon;
    };
}