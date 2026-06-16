#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/CoreUObject/UObject/FStrProperty.hpp"
#include "Unreal/Property/FTextProperty.hpp"
#include "SDK/Helper/PropertyHelper.h"
#include "Loader/WildcardFilter/WildcardStringFilter.h"

using namespace RC;
using namespace RC::Unreal;
using namespace Palworld;

namespace PS {
    WildcardStringFilter::WildcardStringFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data) 
        : WildcardFilter(sourceProperty, data)
    {
        Validate();

        if (m_value.is_array())
        {
            for (const nlohmann::json& value : m_value)
            {
                RC::StringType filterString = RC::to_generic_string(value.get<std::string>());
                m_filterStrings.push_back(filterString);
            }
        }
        else
        {
            RC::StringType filterString = RC::to_generic_string(m_value.get<std::string>());
            m_filterStrings.push_back(filterString);
        }
    }

    bool WildcardStringFilter::Compare(void* container)
    {
        void* srcData = m_sourceProperty->ContainerPtrToValuePtr<void>(container);
        RC::StringType sourceString{};

        if (FStrProperty* strProp = PropertyHelper::CastProperty<FStrProperty>(m_sourceProperty))
        {
            sourceString = *strProp->GetPropertyValue(srcData);
        }
        else if (FNameProperty* nameProp = PropertyHelper::CastProperty<FNameProperty>(m_sourceProperty))
        {
            sourceString = nameProp->GetPropertyValue(srcData).ToString();
        }
        else if (FTextProperty* textProp = PropertyHelper::CastProperty<FTextProperty>(m_sourceProperty))
        {
            sourceString = textProp->GetPropertyValue(srcData).ToString();
        }

        for (const RC::StringType& filterString : m_filterStrings)
        {
            if (CompareString(sourceString, filterString))
            {
                return true;
            }
        }

        return m_filterStrings.empty();
    }

    void WildcardStringFilter::Validate()
    {
        if (!m_value.is_string() && !m_value.is_array())
        {
            throw std::runtime_error(RC::fmt("Value for '%S' must be a string or an array of strings.", m_sourceProperty->GetName().c_str()));
        }

        if (m_value.is_array())
        {
            int index = 0;
            for (auto& value : m_value.get<nlohmann::json::array_t>())
            {
                if (!value.is_string())
                {
                    throw std::runtime_error(RC::fmt("Array value for '%S' at index %d must be a string.", m_sourceProperty->GetName().c_str(), index));
                }
                index++;
            }
        }
    }

    bool WildcardStringFilter::CompareString(RC::StringViewType a, RC::StringViewType b)
    {
        switch (m_operationType)
        {
        case PS::EWildcardOperationType::Equals:
            return a == b;
        case PS::EWildcardOperationType::Contains:
            return a.contains(b);
        case PS::EWildcardOperationType::StartsWith:
            return a.starts_with(b);
        case PS::EWildcardOperationType::EndsWith:
            return a.ends_with(b);
        }

        throw std::runtime_error(RC::fmt("'%S' had an unsupported operation for a string.", m_sourceProperty->GetName().c_str()));
    }
}