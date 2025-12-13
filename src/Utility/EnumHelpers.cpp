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
        for (const auto& enumPair : enum_->GetNames())
        {
            if (enumPair.Key.ToString() == enumKey)
            {
                return enumPair.Value;
            }
        }

        PS::Log<LogLevel::Warning>(STR("Enum '{}' doesn't exist."), enumKey);

        return 0;
    }
}