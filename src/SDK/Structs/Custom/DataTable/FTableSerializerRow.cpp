#include "SDK/Structs/Custom/DataTable/FTableSerializerRow.h"
#include "Unreal/Engine/UDataTable.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    FTableSerializerRow::~FTableSerializerRow()
    {
        if (m_mode == ETableSerializeMode::Add)
        {
            m_table->AddRow(m_rowId, *static_cast<FTableRowBase*>(m_data));

            m_struct->DestroyStruct(m_data);
            FMemory::Free(m_data);
        }
    }

    FTableSerializerRow::FTableSerializerRow(RC::Unreal::UDataTable* table, const RC::Unreal::FName& rowId, ETableSerializeMode mode)
        : m_table(table), m_mode(mode), m_rowId(rowId)
    {
        auto rowStruct = table->GetRowStruct().Get();

        if (mode == ETableSerializeMode::Add)
        {
            m_data = FMemory::Malloc(rowStruct->GetStructureSize());
            m_struct = rowStruct;
            m_struct->InitializeStruct(m_data);
        }
        else if (mode == ETableSerializeMode::Edit)
        {
            m_struct = rowStruct;

            auto data = table->FindRowUnchecked(rowId);
            if (!data)
            {
                Output::send<LogLevel::Warning>(STR("FTableSerializerRow: Failed to find Row '{}'.\n"), rowId.ToString());
                return;
            }

            m_data = data;
        }
    }
}