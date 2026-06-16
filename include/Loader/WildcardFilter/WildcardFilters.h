#pragma once

#include <vector>
#include "nlohmann/json.hpp"
#include "Loader/WildcardFilter/WildcardFilter.h"

namespace PS {
    class WildcardFilters {
    public:
        void Parse(const nlohmann::json& data, RC::Unreal::UScriptStruct* rowStruct);

        bool Match(void* container);

        bool IsEmpty();
    private:
        std::vector<std::unique_ptr<WildcardFilter>> m_filters;

        std::unique_ptr<WildcardFilter> CreateFilter(const nlohmann::json& filterData, RC::Unreal::UScriptStruct* rowStruct);
    };
}