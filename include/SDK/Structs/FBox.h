#pragma once

#include "Unreal/UnrealCoreStructs.hpp"

namespace UECustom {
    struct FBox {
        RC::Unreal::FVector Min;
        RC::Unreal::FVector Max;
        bool IsValid;

        inline bool IsInside(const RC::Unreal::FVector& In) const
        {
            return ((In.X() > Min.X()) && (In.X() < Max.X()) && (In.Y() > Min.Y()) && (In.Y() < Max.Y()) && (In.Z() > Min.Z()) && (In.Z() < Max.Z()));
        }
    };
}