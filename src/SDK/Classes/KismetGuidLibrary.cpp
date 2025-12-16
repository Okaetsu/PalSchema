#include "SDK/Classes/KismetGuidLibrary.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/UFunction.hpp"
#include "Utility/Logging.h"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    UECustom::FGuid UKismetGuidLibrary::NewGuid()
    {
        static auto Function = UECustom::UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, TEXT("/Script/Engine.KismetGuidLibrary:NewGuid"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("Function /Script/Engine.KismetGuidLibrary:NewGuid was invalid.\n"));
            return {};
        }

        struct {
            UECustom::FGuid ReturnValue;
        }params;

        GetDefaultObj()->ProcessEvent(Function, &params);

        return params.ReturnValue;
    }

    RC::Unreal::FString UKismetGuidLibrary::Conv_GuidToString(const UECustom::FGuid& Guid)
    {
        static auto Function = UECustom::UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, TEXT("/Script/Engine.KismetGuidLibrary:Conv_GuidToString"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("Function /Script/Engine.KismetGuidLibrary:Conv_GuidToString was invalid.\n"));
            return {};
        }

        struct {
            UECustom::FGuid Guid;
            RC::Unreal::FString ReturnValue;
        }params;

        params.Guid = Guid;

        GetDefaultObj()->ProcessEvent(Function, &params);

        return params.ReturnValue;
    }

    UKismetGuidLibrary* UKismetGuidLibrary::GetDefaultObj()
    {
        static auto Self = UECustom::UObjectGlobals::StaticFindObject<UKismetGuidLibrary*>(nullptr, nullptr, TEXT("/Script/Engine.Default__KismetGuidLibrary"));
        return Self;
    }
}
