#pragma once

#include "SDK/Structs/Reflected/BaseReflectedStruct.h"

namespace Palworld {
    struct FPalSpawnerOneTribeInfo : UECustom::BaseReflectedStruct {
    public:
        FPalSpawnerOneTribeInfo();
        FPalSpawnerOneTribeInfo(void* Data);

        void SetPalId(const RC::Unreal::FName& value);
        void SetNPCID(const RC::Unreal::FName& value);
        void SetLevel(int value);
        void SetLevelMax(int value);
        void SetNum(int value);
        void SetNumMax(int value);

        virtual RC::Unreal::UScriptStruct* StaticStruct() final;
    };
}