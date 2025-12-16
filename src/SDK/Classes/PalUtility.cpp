#include "SDK/Classes/PalUtility.h"
#include "Unreal/UFunction.hpp"
#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Utility/Logging.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    UPalMasterDataTables* UPalUtility::GetMasterDataTables(UObject* WorldContextObject)
    {
        static auto Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Pal.PalUtility:GetMasterDataTables"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("/Script/Pal.PalUtility:GetMasterDataTables not found, unable to call UPalUtility::GetMasterDataTables\n"));
            return nullptr;
        }

        struct LocalParams {
            UObject* WorldContextObject;
            UPalMasterDataTables* ReturnValue;
        }params;

        params.WorldContextObject = WorldContextObject;

        GetDefaultObject()->ProcessEvent(Function, &params);

        return params.ReturnValue;
    }

    void UPalUtility::Alert(UObject* WorldContextObject, const FText& Message)
    {
        static auto Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Pal.PalUtility:Alert"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("/Script/Pal.PalUtility:Alert not found, unable to call UPalUtility::Alert\n"));
            return;
        }

        struct LocalParams {
            UObject* WorldContextObject;
            FText Message;
        }params;

        params.WorldContextObject = WorldContextObject;
        params.Message = Message;

        GetDefaultObject()->ProcessEvent(Function, &params);
    }

    UPalItemIDManager* UPalUtility::GetItemIDManager(RC::Unreal::UObject* WorldContextObject)
    {
        static auto Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Pal.PalUtility:GetItemIDManager"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("/Script/Pal.PalUtility:GetItemIDManager not found, unable to call UPalUtility::GetItemIDManager\n"));
            return nullptr;
        }

        struct LocalParams {
            UObject* WorldContextObject;
            UPalItemIDManager* ReturnValue;
        }params;

        params.WorldContextObject = WorldContextObject;

        GetDefaultObject()->ProcessEvent(Function, &params);

        return params.ReturnValue;
    }

    UPalUtility* UPalUtility::GetDefaultObject()
    {
        static auto Class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Pal.PalUtility"));
        return static_cast<UPalUtility*>(Class->GetClassDefaultObject());
    }
}