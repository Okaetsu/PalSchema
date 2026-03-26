#pragma once

namespace Palworld {
    struct FPalDynamicItemId {
        RC::Unreal::FGuid CreatedWorldId{};
        RC::Unreal::FGuid LocalIdInCreatedWorld{};
    };
}