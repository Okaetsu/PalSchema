#pragma once

#include "Unreal/UnrealCoreStructs.hpp"

namespace UECustom {
    // Custom implementation that allows it to be used in std::set
    // RC::Unreal::TSet would've been better, but it seems like UE4SS doesn't have a GetTypeHash override for RC::Unreal::FVector ?
    struct FVector {
        FVector(const double& x, const double& y, const double& z) : X(x), Y(y), Z(z) {};
        FVector(const RC::Unreal::FVector& vector) : X(vector.X()), Y(vector.Y()), Z(vector.Z()) {};

        double X;
        double Y;
        double Z;

        bool operator<(const FVector& Other) const
        {
            if (X != Other.X) return X < Other.X;
            if (Y != Other.Y) return Y < Other.Y;
            return Z < Other.Z;
        }
    };
}