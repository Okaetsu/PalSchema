#pragma once

#include "Unreal/Property/FEnumProperty.hpp"

namespace Palworld {
    class UPalEnum;

    class FPalEnumProperty : public RC::Unreal::FEnumProperty
    {
    public:
        RC::Unreal::int64 GetPropertyValue(void* Container);

        UPalEnum* GetPalEnum();
    };
}