#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/Core/Containers/Array.hpp"

namespace UECustom {
    class UCompositeDataTable : public RC::Unreal::UDataTable {
    public:
        static RC::Unreal::UClass* StaticClass();
    public:
        RC::Unreal::TArray<RC::Unreal::TObjectPtr<RC::Unreal::UDataTable>> GetParentTables();
    };
}