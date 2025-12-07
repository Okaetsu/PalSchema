#pragma once

#include "Unreal/NameTypes.hpp"
#include "SDK/Structs/Custom/DataTable/FTableSerializerRow.h"

namespace RC::Unreal {
    class UDataTable;
}

namespace Palworld {
    class TableSerializer {
    public:
        TableSerializer(RC::Unreal::UDataTable* table);

        FTableSerializerRow* Add(const RC::Unreal::FName& rowId);

        FTableSerializerRow* Edit(const RC::Unreal::FName& rowId);
    private:
        RC::Unreal::UDataTable* m_table = nullptr;
        std::vector<std::unique_ptr<FTableSerializerRow>> m_rows;
    };
}