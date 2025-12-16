#include "SDK/Classes/PalItemIDManager.h"
#include "SDK/Classes/PalStaticItemDataTable.h"
#include "Unreal/UFunction.hpp"
#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Utility/Logging.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    UPalStaticItemDataTable* UPalItemIDManager::GetStaticItemDataTable()
    {
        auto StaticItemDataTable = this->GetValuePtrByPropertyNameInChain<UPalStaticItemDataTable*>(STR("StaticItemDataTable"));
        if (StaticItemDataTable)
        {
            return *StaticItemDataTable;
        }
        return nullptr;
    }

    UObject* UPalItemIDManager::GetStaticItemData(const FName& StaticItemId)
    {
        static auto Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Pal.PalItemIDManager:GetStaticItemData"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("/Script/Pal.PalItemIDManager:GetStaticItemData not found, unable to call UPalItemIDManager::GetStaticItemData\n"));
            return nullptr;
        }

        struct LocalParams {
            FName StaticItemId;
            UObject* ReturnValue;
        }params;

        params.StaticItemId = StaticItemId;

        this->ProcessEvent(Function, &params);

        return params.ReturnValue;
    }
}