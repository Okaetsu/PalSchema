#include "Utility/JsonHelpers.h"
#include "Unreal/Core/HAL/Platform.hpp"
#include "Unreal/NameTypes.hpp"
#include "Unreal/UnrealCoreStructs.hpp"
#include "Unreal/Rotator.hpp"
#include "nlohmann/json.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace PS::JsonHelpers {
    bool FieldExists(const nlohmann::json& data, const std::string& fieldName)
    {
        return data.contains(fieldName);
    }

    void ValidateFieldExists(const nlohmann::json& data, const std::string& fieldName)
    {
        if (!data.contains(fieldName))
        {
            throw std::runtime_error(std::format("Missing a required field of '{}'.", fieldName));
        }
    }

    void ParseRotator(const nlohmann::json& value, const std::string& fieldName, FRotator& outValue)
    {
        auto& field = value.at(fieldName);

        if (!field.is_object() || !field.contains("Pitch") || !field.contains("Yaw") || !field.contains("Roll"))
        {
            throw std::runtime_error(std::format("FRotator '{}' must be an object with fields 'Pitch', 'Yaw' and 'Roll'.", fieldName));
        }

        double pitch, yaw, roll;
        ParseDouble(field, "Pitch", pitch);
        ParseDouble(field, "Yaw", yaw);
        ParseDouble(field, "Roll", roll);

        outValue = FRotator{ pitch, yaw, roll };
    }

    void ParseVector(const nlohmann::json& value, const std::string& fieldName, FVector& outValue)
    {
        auto& field = value.at(fieldName);

        if (!field.is_object() || !field.contains("X") || !field.contains("Y") || !field.contains("Z"))
        {
            throw std::runtime_error(std::format("FVector '{}' must be an object with fields 'X', 'Y' and 'Z'.", fieldName));
        }

        double x, y, z;
        ParseDouble(field, "X", x);
        ParseDouble(field, "Y", y);
        ParseDouble(field, "Z", z);

        outValue = FVector{ x, y, z };
    }

    void ParseFName(const nlohmann::json& value, const std::string& fieldName, FName& outValue)
    {
        std::string parsedString;
        ParseString(value, fieldName, parsedString);

        auto wideString = RC::to_generic_string(parsedString);

        outValue = FName(wideString, FNAME_Add);
    }

    void ParseDouble(const nlohmann::json& value, const std::string& fieldName, double& outValue)
    {
        auto& field = value.at(fieldName);

        if (!field.is_number_float())
        {
            throw std::runtime_error(std::format("Value '{}' must be a floating point number.", fieldName));
        }

        outValue = field.get<double>();
    }

    void ParseInteger(const nlohmann::json& value, const std::string& fieldName, int& outValue)
    {
        auto& field = value.at(fieldName);

        if (!field.is_number_integer())
        {
            throw std::runtime_error(std::format("Value '{}' must be an integer.", fieldName));
        }

        outValue = field.get<int>();
    }

    void ParseUInt8(const nlohmann::json& value, const std::string& fieldName, RC::Unreal::uint8& outValue)
    {
        auto& field = value.at(fieldName);

        if (!field.is_number_integer())
        {
            throw std::runtime_error(std::format("Value '{}' must be an integer.", fieldName));
        }

        outValue = field.get<RC::Unreal::uint8>();
    }

    void ParseString(const nlohmann::json& value, const std::string& fieldName, std::string& outValue)
    {
        auto& field = value.at(fieldName);

        if (!field.is_string())
        {
            throw std::runtime_error(std::format("Value '{}' must be a string.", fieldName));
        }

        outValue = field.get<std::string>();
    }
}