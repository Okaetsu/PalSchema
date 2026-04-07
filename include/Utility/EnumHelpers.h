#pragma once

#include "Unreal/Core/HAL/Platform.hpp"
#include "Unreal/NameTypes.hpp"

namespace RC::Unreal {
    class UEnum;
}

namespace PS::EnumHelpers {
    RC::Unreal::int64 GetEnumValueByName(RC::Unreal::UEnum* enum_, const std::string& enumString);

    // Expects the enum name to be in the following format: Namespace::Name, e.g. EPalWazaID::TidalWave
    RC::Unreal::int64 GetEnumValueByName(RC::Unreal::UEnum* enumClass, const RC::Unreal::FName& enumName);
}