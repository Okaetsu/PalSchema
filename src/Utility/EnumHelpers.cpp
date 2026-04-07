#include "Utility/EnumHelpers.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include "Unreal/UEnum.hpp"

using namespace RC;

namespace PS::EnumHelpers {
    RC::Unreal::int64 GetEnumValueByName(RC::Unreal::UEnum* enum_, const std::string& enumString)
    {
        std::string enumStringFixed = enumString;

        auto enumName = enum_->GetName();
        if (!enumStringFixed.contains("::"))
        {
            enumStringFixed = std::format("{}::{}", RC::to_string(enumName), enumStringFixed);
        }

        auto enumKey = RC::to_generic_string(enumStringFixed);
        for (const auto& enumPair : enum_->GetEnumNames())
        {
            if (enumPair.Key.ToString() == enumKey)
            {
                return enumPair.Value;
            }
        }

        PS::Log<LogLevel::Warning>(STR("Enum '{}' doesn't exist."), enumKey);

        return 0;
    }

    RC::Unreal::int64 GetEnumValueByName(RC::Unreal::UEnum* enumClass, const RC::Unreal::FName& enumName)
    {
        for (const auto& enumPair : enumClass->GetEnumNames())
        {
            if (enumPair.Key == enumName)
            {
                return enumPair.Value;
            }
        }

        PS::Log<LogLevel::Warning>(STR("Enum '{}' doesn't exist."), enumName.ToString());

        return 0;
    }
}