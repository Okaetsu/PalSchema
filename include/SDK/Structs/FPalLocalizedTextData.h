#pragma once

#include "Unreal/FText.hpp"
#include "Unreal/Engine/UDataTable.hpp"

namespace Palworld {
    struct FPalLocalizedTextData final : public RC::Unreal::FTableRowBase
    {
    public:
        RC::Unreal::FText TextData;
    };
}