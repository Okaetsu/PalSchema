#include "SDK/Classes/APalSpawnerStandard.h"
#include "SDK/Helper/PropertyHelper.h"
#include "SDK/Structs/Custom/FScriptArrayHelper.h"
#include "SDK/Structs/Reflected/PalSpawnerGroupInfo.h"
#include "Unreal/UClass.hpp"
#include "Unreal/Core/Containers/ScriptArray.hpp"
#include "Unreal/FProperty.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    void APalSpawnerStandard::AddSpawnerGroup(const PalSpawnerGroup& spawnerGroup) {
        auto spawnGroupListProperty = PropertyHelper::GetPropertyByName(this->GetClassPrivate(), TEXT("SpawnGroupList"));
        if (!spawnGroupListProperty)
        {
            throw std::runtime_error("Could not get SpawnGroupList in APalSpawnerStandard.");
        }

        auto spawnGroupList = spawnGroupListProperty->ContainerPtrToValuePtr<void>(this);

        auto scriptArray = static_cast<FScriptArray*>(spawnGroupList);
        auto scriptArrayHelper = UECustom::FScriptArrayHelper(scriptArray, static_cast<FArrayProperty*>(spawnGroupListProperty));

        UECustom::FManagedValue valuePtr;
        scriptArrayHelper.InitializeValue(valuePtr);

        FPalSpawnerGroupInfo spawnerGroupInfo(valuePtr.GetData());
        spawnerGroupInfo.SetWeight(spawnerGroup.Weight);
        spawnerGroupInfo.SetOnlyTime(spawnerGroup.OnlyTime);
        spawnerGroupInfo.SetOnlyWeather(spawnerGroup.OnlyWeather);

        for (auto& palListInfo : spawnerGroup.PalList)
        {
            spawnerGroupInfo.AddPal(palListInfo);
        }

        scriptArrayHelper.Add(valuePtr);
    }

    void APalSpawnerStandard::SetSpawnerName(const RC::Unreal::FName& spawnerName)
    {
        auto property = PropertyHelper::GetPropertyByName(this->GetClassPrivate(), TEXT("SpawnerName"));
        *property->ContainerPtrToValuePtr<RC::Unreal::FName>(this) = spawnerName;
    }

    void APalSpawnerStandard::SetSpawnerType(const RC::Unreal::uint8& spawnerType)
    {
        auto property = PropertyHelper::GetPropertyByName(this->GetClassPrivate(), TEXT("SpawnerType"));
        *property->ContainerPtrToValuePtr<RC::Unreal::uint8>(this) = spawnerType;
    }
}