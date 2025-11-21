#pragma once

#include <unordered_map>
#include <string>

namespace RC::Unreal {
    class UDataTable;
}

namespace UECustom {
    class UDataTableStore {
    private:
        static inline std::unordered_map<std::string, RC::Unreal::UDataTable*> TableMap;
    public:
        static RC::Unreal::UDataTable* GetTableByName(const std::string& Name);

        static void Store(const std::string& Name, RC::Unreal::UDataTable* Table);

        static void Store(RC::Unreal::UDataTable* Table);

        static void Initialize();
    };
}