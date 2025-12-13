#pragma once

#include "Helpers/String.hpp"
#include "Unreal/UClass.hpp"
#include "Unreal/NameTypes.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "SDK/Helper/PropertyHelper.h"

namespace RC::Unreal {
    class UDataTable;
}

namespace Palworld {
    // When adding a row, the row is added to the table once this object destructs.
    // When editing a row, changes are applied to the table immediately.
    struct FTableSerializerRow {
    public:
        enum class ETableSerializeMode {
            None,
            Add,
            Edit
        };

        ~FTableSerializerRow();
        FTableSerializerRow(RC::Unreal::UDataTable* table, const RC::Unreal::FName& rowId, ETableSerializeMode mode);

        template <typename T>
        void SetValue(const RC::StringType& propertyName, const T& value)
        {
            auto property = PropertyHelper::GetPropertyByName(m_struct, propertyName);
            if (property && m_data)
            {
                auto valuePtr = property->ContainerPtrToValuePtr<void>(m_data);
                RC::Unreal::FMemory::Memcpy(valuePtr, &value, sizeof(T));
            }
        }
    private:
        RC::Unreal::UDataTable* m_table = nullptr;
        RC::Unreal::UScriptStruct* m_struct = nullptr;
        RC::Unreal::FName m_rowId = RC::Unreal::NAME_None;
        void* m_data = nullptr;
        ETableSerializeMode m_mode = ETableSerializeMode::None;
    };
}