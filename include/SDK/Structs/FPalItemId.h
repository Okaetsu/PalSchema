#pragma once

#include "SDK/Structs/FPalDynamicItemId.h"

namespace Palworld {
    struct FPalItemId {
        RC::Unreal::FName StaticId;
        FPalDynamicItemId DynamicId;
    };
}