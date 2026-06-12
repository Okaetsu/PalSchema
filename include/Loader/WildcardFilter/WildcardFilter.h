#pragma once

#include "nlohmann/json.hpp"
#include "Unreal/NameTypes.hpp"

namespace PS {
    enum class EWildcardOperationType : RC::Unreal::uint8 {
        Unknown,
        Equals,
        GreaterThan,
        GreaterThanOrEqual,
        LessThan,
        LessThanOrEqual,
        Contains,
        StartsWith,
        EndsWith
    };

    class WildcardFilter {
    public:
        WildcardFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data);
        virtual ~WildcardFilter() {};

        void Parse(const nlohmann::json& data);

        virtual bool Compare(void* container) = 0;
    protected:
        virtual void Validate() = 0;

        RC::Unreal::FProperty* m_sourceProperty = nullptr;
        RC::Unreal::FName m_fieldName = RC::Unreal::NAME_None;
        EWildcardOperationType m_operationType{};
        nlohmann::json m_value;
    private:
        EWildcardOperationType GetOperationTypeFromString(const std::string& operationString);
    };
}