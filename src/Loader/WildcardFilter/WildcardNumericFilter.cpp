#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "SDK/Helper/PropertyHelper.h"
#include "Loader/WildcardFilter/WildcardNumericFilter.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS {
    WildcardNumericFilter::WildcardNumericFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data) 
        : WildcardFilter(sourceProperty, data)
    {
        Validate();
    }

    bool WildcardNumericFilter::Compare(void* container)
    {
        FNumericProperty* numericProp = Palworld::PropertyHelper::CastProperty<FNumericProperty>(m_sourceProperty);
        void* srcData = m_sourceProperty->ContainerPtrToValuePtr<void>(container);

        const auto doesFilterMatch = [&](const nlohmann::json& value) -> bool {
            if (numericProp->IsInteger())
            {
                int64 sourceInt = numericProp->GetSignedIntPropertyValue(srcData);
                int64 filterInt = value.get<int64>();
                return CompareNumber(sourceInt, filterInt);
            }
            else if (numericProp->IsFloatingPoint())
            {
                double sourceFloat = numericProp->GetFloatingPointPropertyValue(srcData);
                double filterFloat = value.get<double>();
                return CompareNumber(sourceFloat, filterFloat);
            }
            else
            {
                throw std::runtime_error(RC::fmt("Unsupported numeric type in '%S'", m_sourceProperty->GetName().c_str()));
            }
        };

        if (m_value.is_array())
        {
            auto filterArray = m_value.get<nlohmann::json::array_t>();
            for (const nlohmann::json& value : filterArray)
            {
                if (doesFilterMatch(value))
                {
                    return true;
                }
            }

            return filterArray.empty();
        }

        return doesFilterMatch(m_value);
    }

    void WildcardNumericFilter::Validate()
    {
        if (!m_value.is_number() && !m_value.is_array())
        {
            throw std::runtime_error(RC::fmt("Value for '%S' must be a number or an array of numbers.", m_sourceProperty->GetName().c_str()));
        }

        if (m_value.is_array())
        {
            int index = 0;
            for (auto& value : m_value.get<nlohmann::json::array_t>())
            {
                if (!value.is_number())
                {
                    throw std::runtime_error(RC::fmt("Array value for '%S' at index %d must be a number.", m_sourceProperty->GetName().c_str(), index));
                }
                index++;
            }
        }
    }
}