#pragma once

#include "Unreal/Core/HAL/Platform.hpp"
#include "Unreal/NameTypes.hpp"

namespace RC::Unreal {
    class UEnum;
}

namespace PS::EnumHelpers {
    RC::Unreal::int64 GetEnumValueByName(RC::Unreal::UEnum* enum_, const std::string& enumString);
}