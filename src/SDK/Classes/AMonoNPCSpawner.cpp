#include "SDK/Classes/AMonoNPCSpawner.h"
#include "Unreal/UFunction.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Utility/Logging.h"

using namespace RC::Unreal;

namespace Palworld {
    int& AMonoNPCSpawner::GetLevel()
    {
        auto Value = this->GetValuePtrByPropertyNameInChain<int>(TEXT("Level"));
        return *Value;
    }

    RC::Unreal::FName& AMonoNPCSpawner::GetHumanName()
    {
        auto Value = this->GetValuePtrByPropertyNameInChain<RC::Unreal::FName>(TEXT("HumanName"));
        return *Value;
    }

    RC::Unreal::FName& AMonoNPCSpawner::GetCharaName()
    {
        auto Value = this->GetValuePtrByPropertyNameInChain<RC::Unreal::FName>(TEXT("CharaName"));
        return *Value;
    }

    void AMonoNPCSpawner::Spawn()
    {
        auto Function = this->GetFunctionByNameInChain(TEXT("Spawn"));

        if (!Function)
        {
            PS::Log<RC::LogLevel::Error>(STR("Failed to execute 'Spawn', could not find function /Game/Pal/Blueprint/Spawner/BP_MonoNPCSpawner.BP_MonoNPCSpawner_C:Spawn.\n"));
            return;
        }

        this->ProcessEvent(Function, nullptr);
    }
}