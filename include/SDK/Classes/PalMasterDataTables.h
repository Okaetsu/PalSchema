#pragma once

#include "Unreal/UObject.hpp"

namespace RC::Unreal {
    class UDataTable;
}

namespace Palworld {
    class UPalMasterDataTables : public RC::Unreal::UObject {
    public:
        RC::Unreal::UDataTable* GetItemRecipeDataTable();
    };
}