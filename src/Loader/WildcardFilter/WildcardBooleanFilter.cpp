#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "SDK/Helper/PropertyHelper.h"
#include "Loader/WildcardFilter/WildcardBooleanFilter.h"

using namespace RC;
using namespace RC::Unreal;
using namespace Palworld;

namespace PS {
    WildcardBooleanFilter::WildcardBooleanFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data) 
        : WildcardFilter(sourceProperty, data)
    {
        Validate();
    }

    bool WildcardBooleanFilter::Compare(void* container)
    {
        bool sourceBool = *m_sourceProperty->ContainerPtrToValuePtr<bool>(container);
        bool filterBool = m_value.get<bool>();

        switch (m_operationType)
        {
        case PS::EWildcardOperationType::Equals:
            return sourceBool == filterBool;
        default:
            throw std::runtime_error("Only equals operation is supported for a boolean filter.");
        }
    }

    void WildcardBooleanFilter::Validate()
    {
        if (m_value.is_array())
        {
            throw std::runtime_error(RC::fmt("'%S' is a boolean property and does not support multiple values.", m_sourceProperty->GetName().c_str()));
        }

        if (!m_value.is_boolean())
        {
            throw std::runtime_error(RC::fmt("'%S' must be a boolean value.", m_sourceProperty->GetName().c_str()));
        }
    }
}