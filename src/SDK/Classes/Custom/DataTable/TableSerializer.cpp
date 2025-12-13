#include "SDK/Classes/Custom/DataTable/TableSerializer.h"
#include "Unreal/Engine/UDataTable.hpp"

namespace Palworld {
    TableSerializer::TableSerializer(RC::Unreal::UDataTable* table) : m_table(table)
    {

    }

    FTableSerializerRow* TableSerializer::Add(const RC::Unreal::FName& rowId)
    {
        m_rows.push_back(std::make_unique<FTableSerializerRow>(m_table, rowId, FTableSerializerRow::ETableSerializeMode::Add));
        auto newRow = m_rows.back().get();
        return newRow;
    }

    FTableSerializerRow* TableSerializer::Edit(const RC::Unreal::FName& rowId)
    {
        auto serializeRow = std::make_unique<FTableSerializerRow>(m_table, rowId, FTableSerializerRow::ETableSerializeMode::Edit);
        m_rows.push_back(std::move(serializeRow));

        auto newRow = m_rows.back().get();
        return newRow;
    }
}