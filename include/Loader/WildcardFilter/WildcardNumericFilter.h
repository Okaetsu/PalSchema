#pragma once

#include "Loader/WildcardFilter/WildcardFilter.h"

namespace PS {
    class WildcardNumericFilter : public WildcardFilter {
    public:
        WildcardNumericFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data);

        virtual bool Compare(void* container) override final;
    private:
        virtual void Validate() override final;

        template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
        bool CompareNumber(const T& a, const T& b)
        {
            switch (m_operationType)
            {
            case EWildcardOperationType::Equals:
                return a == b;
            case EWildcardOperationType::GreaterThan:
                return a > b;
            case EWildcardOperationType::GreaterThanOrEqual:
                return a >= b;
            case EWildcardOperationType::LessThan:
                return a < b;
            case EWildcardOperationType::LessThanOrEqual:
                return a <= b;
            }

            throw std::runtime_error(RC::fmt("'%S' had an unsupported operation for a number.", m_sourceProperty->GetName().c_str()));
        }
    };
}