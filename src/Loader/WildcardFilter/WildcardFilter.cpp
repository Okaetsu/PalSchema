#include "Loader/WildcardFilter/WildcardFilter.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace PS {
    WildcardFilter::WildcardFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data) : m_sourceProperty(sourceProperty)
    {
        Parse(data);
    }

    void WildcardFilter::Parse(const nlohmann::json& data)
    {
        if (!m_sourceProperty)
        {
            throw std::runtime_error("Internal Error: Failed to parse wildcard filter, source property was NULL.");
        }

        if (!data.contains("Operation"))
        {
            throw std::runtime_error("Failed to parse wildcard filter, property 'Operation' was not specified.");
        }

        auto& operation = data.at("Operation");
        if (!operation.is_string())
        {
            throw std::runtime_error("Failed to parse wildcard filter, property 'Operation' was not a string.");
        }

        auto operationString = operation.get<std::string>();
        auto operationValue = GetOperationTypeFromString(operationString);
        if (operationValue == EWildcardOperationType::Unknown)
        {
            throw std::runtime_error(
                std::format("Failed to parse wildcard filter, property 'Operation' was supplied an invalid value of {}.", operationString));
        }

        m_operationType = operationValue;

        if (data.contains("Value"))
        {
            if (data.at("Value").is_array())
            {
                throw std::runtime_error("Failed to parse wildcard filter, 'Value' is meant for single values. Use 'Values' to supply multiple values.");
            }

            m_value = data.at("Value");
        }
        else if (data.contains("Values"))
        {
            if (!data.at("Values").is_array())
            {
                throw std::runtime_error("Failed to parse wildcard filter, 'Values' is meant for an array of values. Use 'Value' to supply a single value.");
            }

            m_value = data.at("Values");
        }
        else
        {
            throw std::runtime_error("Failed to parse wildcard filter, must specify either 'Value' or 'Values' field.");
        }
    }

    EWildcardOperationType WildcardFilter::GetOperationTypeFromString(const std::string& operationString)
    {
        if (operationString == "Equals" || operationString == "Equal" || operationString == "==")
        {
            return EWildcardOperationType::Equals;
        }
        else if (operationString == "GreaterThan" || operationString == ">")
        {
            return EWildcardOperationType::GreaterThan;
        }
        else if (operationString == "GreaterThanOrEqual" || operationString == ">=")
        {
            return EWildcardOperationType::GreaterThanOrEqual;
        }
        else if (operationString == "LessThan" || operationString == "<")
        {
            return EWildcardOperationType::LessThan;
        }
        else if (operationString == "LessThanOrEqual" || operationString == "<=")
        {
            return EWildcardOperationType::LessThanOrEqual;
        }
        else if (operationString == "Contains")
        {
            return EWildcardOperationType::Contains;
        }
        else if (operationString == "StartsWith")
        {
            return EWildcardOperationType::StartsWith;
        }
        else if (operationString == "EndsWith")
        {
            return EWildcardOperationType::EndsWith;
        }
        else
        {
            return EWildcardOperationType::Unknown;
        }
    }
}