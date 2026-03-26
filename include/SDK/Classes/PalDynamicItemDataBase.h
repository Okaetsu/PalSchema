#pragma once

#include "Unreal/CoreUObject/UObject/UnrealType.hpp"

namespace Palworld {
    class UPalDynamicItemDataBase : public RC::Unreal::UObject {
    public:
        bool& GetIgnoreOnSave();
    };
}