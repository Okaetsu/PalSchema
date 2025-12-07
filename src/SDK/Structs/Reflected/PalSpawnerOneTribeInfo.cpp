#include "SDK/Structs/Reflected/PalSpawnerOneTribeInfo.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/Property/FNumericProperty.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    FPalSpawnerOneTribeInfo::FPalSpawnerOneTribeInfo() : BaseReflectedStruct(StaticStruct()) {}
    FPalSpawnerOneTribeInfo::FPalSpawnerOneTribeInfo(void* data) : BaseReflectedStruct(StaticStruct(), data) {};

    void FPalSpawnerOneTribeInfo::SetPalId(const RC::Unreal::FName& value)
    {
        auto property = GetProperty(STR("PalId"));
        SetPropertyValue<RC::Unreal::FName>(property, value);
    }

    void FPalSpawnerOneTribeInfo::SetNPCID(const RC::Unreal::FName& value)
    {
        auto property = GetProperty(STR("NPCID"));
        SetPropertyValue<RC::Unreal::FName>(property, value);
    }

    void FPalSpawnerOneTribeInfo::SetLevel(int value)
    {
        auto property = GetPropertyChecked<FNumericProperty>(STR("Level"));
        SetPropertyValue<int>(property, value);
    }

    void FPalSpawnerOneTribeInfo::SetLevelMax(int value)
    {
        auto property = GetPropertyChecked<FNumericProperty>(STR("Level_Max"));
        SetPropertyValue<int>(property, value);
    }

    void FPalSpawnerOneTribeInfo::SetNum(int value)
    {
        auto property = GetPropertyChecked<FNumericProperty>(STR("Num"));
        SetPropertyValue<int>(property, value);
    }

    void FPalSpawnerOneTribeInfo::SetNumMax(int value)
    {
        auto property = GetPropertyChecked<FNumericProperty>(STR("Num_Max"));
        SetPropertyValue<int>(property, value);
    }

    RC::Unreal::UScriptStruct* FPalSpawnerOneTribeInfo::StaticStruct()
    {
        static auto Struct = UECustom::UObjectGlobals::StaticFindObject<UScriptStruct*>(nullptr, nullptr, STR("/Script/Pal.PalSpawnerOneTribeInfo"));
        return Struct;
    }
}