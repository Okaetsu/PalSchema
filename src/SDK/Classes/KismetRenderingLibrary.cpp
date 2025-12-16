#include "SDK/Classes/KismetRenderingLibrary.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/UFunction.hpp"
#include "Utility/Logging.h"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    RC::Unreal::UObject* UKismetRenderingLibrary::ImportFileAsTexture2D(RC::Unreal::UObject* WorldContextObject, const RC::Unreal::FString& Filename)
    {
        static auto Function = UECustom::UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, TEXT("/Script/Engine.KismetRenderingLibrary:ImportFileAsTexture2D"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("Function /Script/Engine.KismetRenderingLibrary:ImportFileAsTexture2D was invalid.\n"));
            return {};
        }

        struct {
            RC::Unreal::UObject* WorldContextObject;
            RC::Unreal::FString Filename;
            RC::Unreal::UObject* ReturnValue;
        }params;

        params.WorldContextObject = WorldContextObject;
        params.Filename = Filename;

        GetDefaultObj()->ProcessEvent(Function, &params);

        return params.ReturnValue;
    }

    UKismetRenderingLibrary* UKismetRenderingLibrary::GetDefaultObj()
    {
        static auto Self = UECustom::UObjectGlobals::StaticFindObject<UKismetRenderingLibrary*>(nullptr, nullptr, TEXT("/Script/Engine.Default__KismetRenderingLibrary"));
        return Self;
    }
}
