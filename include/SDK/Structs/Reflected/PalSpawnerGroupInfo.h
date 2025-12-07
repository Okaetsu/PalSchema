#pragma once

#include "SDK/Structs/Reflected/BaseReflectedStruct.h"
#include "SDK/Structs/PalListInfo.h"

namespace Palworld {
    struct FPalSpawnerGroupInfo : UECustom::BaseReflectedStruct {
    public:
        FPalSpawnerGroupInfo();
        FPalSpawnerGroupInfo(void* Data);

        void SetWeight(int value);
        void SetOnlyTime(RC::Unreal::uint8 value);
        void SetOnlyWeather(RC::Unreal::uint8 value);
        void AddPal(const PalListInfo& value);

        virtual RC::Unreal::UScriptStruct* StaticStruct() final;
    };
}