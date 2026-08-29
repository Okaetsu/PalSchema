#include "SDK/Classes/KismetSystemLibrary.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/UFunction.hpp"
#include "Utility/Logging.h"
#include <BPMacros.hpp>

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    void UKismetSystemLibrary::CollectGarbage()
    {
        static auto Function = UECustom::UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, TEXT("/Script/Engine.KismetSystemLibrary:CollectGarbage"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("Function /Script/Engine.KismetSystemLibrary:CollectGarbage was invalid.\n"));
            return;
        }

        GetDefaultObj()->ProcessEvent(Function, nullptr);
    }

    UObject* UKismetSystemLibrary::LoadAsset_Blocking(const TSoftObjectPtr<UObject>& Asset, bool bSetRootSet)
    {
        UE_BEGIN_NATIVE_FUNCTION_BODY("/Script/Engine.KismetSystemLibrary:LoadAsset_Blocking")
        UE_SET_STATIC_SELF("/Script/Engine.Default__KismetSystemLibrary")
        UE_COPY_PROPERTY(Asset, TSoftObjectPtr<UObject>)

        UE_CALL_STATIC_FUNCTION()

        UE_RETURN_PROPERTY(UObject*)
    }

    UObject* UKismetSystemLibrary::LoadAsset_Blocking(const RC::StringType& AssetPath, bool bSetRootSet)
    {
        auto SoftObjectPtr = TSoftObjectPtr<UObject>(FSoftObjectPath(FString(AssetPath)));
        auto LoadedAsset = LoadAsset_Blocking(SoftObjectPtr, bSetRootSet);
        return LoadedAsset;
    }

	UKismetSystemLibrary* UKismetSystemLibrary::GetDefaultObj()
	{
		static auto Self = UECustom::UObjectGlobals::StaticFindObject<UKismetSystemLibrary*>(nullptr, nullptr, TEXT("/Script/Engine.Default__KismetSystemLibrary"));
		return Self;
	}
}