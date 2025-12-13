#pragma once

#include <string>
#include "nlohmann/json_fwd.hpp"
#include "Unreal/Core/HAL/Platform.hpp"

namespace RC::Unreal {
    struct FRotator;
    struct FVector;
    class FName;
}

namespace PS::JsonHelpers {
    bool FieldExists(const nlohmann::json& data, const std::string& fieldName);
    void ValidateFieldExists(const nlohmann::json& data, const std::string& fieldName);

    void ParseRotator(const nlohmann::json& value, const std::string& fieldName, RC::Unreal::FRotator& outValue);
    void ParseVector(const nlohmann::json& value, const std::string& fieldName, RC::Unreal::FVector& outValue);
    void ParseFName(const nlohmann::json& value, const std::string& fieldName, RC::Unreal::FName& outValue);
    void ParseDouble(const nlohmann::json& value, const std::string& fieldName, double& outValue);
    void ParseInteger(const nlohmann::json& value, const std::string& fieldName, int& outValue);
    void ParseUInt8(const nlohmann::json& value, const std::string& fieldName, RC::Unreal::uint8& outValue);
    void ParseString(const nlohmann::json& value, const std::string& fieldName, std::string& outValue);
}