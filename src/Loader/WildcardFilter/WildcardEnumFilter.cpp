#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "SDK/Classes/PalEnum.h"
#include "SDK/Classes/Property/PalEnumProperty.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Loader/WildcardFilter/WildcardEnumFilter.h"

using namespace RC;
using namespace RC::Unreal;
using namespace Palworld;

namespace PS {
    WildcardEnumFilter::WildcardEnumFilter(FProperty* sourceProperty, const nlohmann::json& data)
        : WildcardFilter(sourceProperty, data)
    {
        Validate();

        if (m_value.is_array())
        {
            for (const nlohmann::json& value : m_value)
            {
                RC::StringType filterString = RC::to_generic_string(value.get<std::string>());
                FixupEnumString(filterString);
                m_enumFilterStrings.push_back(filterString);
            }
        }
        else
        {
            RC::StringType filterString = RC::to_generic_string(m_value.get<std::string>());
            FixupEnumString(filterString);
            m_enumFilterStrings.push_back(filterString);
        }
    }

    bool WildcardEnumFilter::Compare(void* container)
    {
        FPalEnumProperty* palEnumProp = PropertyHelper::CastProperty<FPalEnumProperty>(m_sourceProperty);
        UPalEnum* palEnum = palEnumProp->GetPalEnum();

        // Uses ContainerPtrToValuePtr internally so we don't need to use ContainerPtrToValuePtr on container first before passing it to GetPropertyValue
        int64 enumIndex = palEnumProp->GetPropertyValue(container);
        FName enumName = palEnum->GetNameByValue(enumIndex);

        for (const RC::StringType& filterString : m_enumFilterStrings)
        {
            if (enumName.ToString() == filterString)
            {
                return true;
            }
        }

        return m_enumFilterStrings.empty();
    }

    void WildcardEnumFilter::Validate()
    {
        if (m_operationType != EWildcardOperationType::Equals)
        {
            throw std::runtime_error(RC::fmt("'%S' is an enum and only supports the equals operation.", m_sourceProperty->GetName().c_str()));
        }

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

    void WildcardEnumFilter::FixupEnumString(RC::StringType& enumString)
    {
        if (!enumString.contains(TEXT("::")))
        {
            FPalEnumProperty* palEnumProp = PropertyHelper::CastProperty<FPalEnumProperty>(m_sourceProperty);
            UPalEnum* palEnum = palEnumProp->GetPalEnum();

            RC::StringType nameSpace = palEnum->GetName();
            RC::StringType enumName = std::format(TEXT("{}::{}"), nameSpace, enumString);

            for (const FEnumNamePair& enumNamePair : palEnum->GetEnumNames())
            {
                if (enumNamePair.Key.ToString() == enumName)
                {
                    enumString = enumName;
                    return;
                }
            }

            throw std::runtime_error(RC::fmt("Invalid enum '%S' supplied to field '%S'.", enumName.c_str(), m_sourceProperty->GetName().c_str()));
        }
    }
}