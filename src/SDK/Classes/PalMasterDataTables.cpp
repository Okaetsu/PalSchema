#include "SDK/Classes/PalMasterDataTables.h"
#include "Unreal/Engine/UDataTable.hpp"

namespace Palworld {
    RC::Unreal::UDataTable* UPalMasterDataTables::GetItemRecipeDataTable()
    {
        auto DataTable = this->GetValuePtrByPropertyNameInChain<RC::Unreal::UDataTable*>(STR("ItemRecipeDataTable"));
        if (DataTable)
        {
            return *DataTable;
        }
        return nullptr;
    }
}