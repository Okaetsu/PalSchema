#include "SDK/Structs/Reflected/PalSpawnerGroupInfo.h"
#include "SDK/Structs/Reflected/PalSpawnerOneTribeInfo.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/Property/FEnumProperty.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    FPalSpawnerGroupInfo::FPalSpawnerGroupInfo() : BaseReflectedStruct(StaticStruct()) {}
    FPalSpawnerGroupInfo::FPalSpawnerGroupInfo(void* data) : BaseReflectedStruct(StaticStruct(), data) {};

    void FPalSpawnerGroupInfo::SetWeight(int value)
    {
        auto property = GetPropertyChecked<FNumericProperty>(STR("Weight"));
        SetPropertyValue<int>(property, value);
    }

    void FPalSpawnerGroupInfo::SetOnlyTime(RC::Unreal::uint8 value)
    {
        auto property = GetPropertyChecked<FEnumProperty>(STR("OnlyTime"));
        SetPropertyValue<RC::Unreal::uint8>(property, value);
    }

    void FPalSpawnerGroupInfo::SetOnlyWeather(RC::Unreal::uint8 value)
    {
        auto property = GetPropertyChecked<FEnumProperty>(STR("OnlyWeather"));
        SetPropertyValue<RC::Unreal::uint8>(property, value);
    }

    void FPalSpawnerGroupInfo::AddPal(const PalListInfo& value)
    {
        auto property = GetPropertyChecked<FArrayProperty>(STR("PalList"));
        auto array = GetArrayPropertyValue(property);

        UECustom::FManagedValue valuePtr;
        array->InitializeValue(valuePtr);

        FPalSpawnerOneTribeInfo tribeInfo(valuePtr.GetData());
        tribeInfo.SetPalId(value.PalId);
        tribeInfo.SetNPCID(value.NPCID);
        tribeInfo.SetLevel(value.Level);
        tribeInfo.SetLevelMax(value.LevelMax);
        tribeInfo.SetNum(value.Num);
        tribeInfo.SetNumMax(value.NumMax);

        array->Add(valuePtr);
    }

    RC::Unreal::UScriptStruct* FPalSpawnerGroupInfo::StaticStruct()
    {
        static auto Struct = UECustom::UObjectGlobals::StaticFindObject<UScriptStruct*>(nullptr, nullptr, STR("/Script/Pal.PalSpawnerGroupInfo"));
        return Struct;
    }
}