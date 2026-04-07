#include "SDK/Classes/KismetSystemLibrary.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/UFunction.hpp"
#include "Utility/Logging.h"

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

    FSoftObjectPath UKismetSystemLibrary::MakeSoftObjectPath(const FString& Path)
	{
		static auto Function = UECustom::UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, TEXT("/Script/Engine.KismetSystemLibrary:MakeSoftObjectPath"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("Function /Script/Engine.KismetSystemLibrary:MakeSoftObjectPath was invalid.\n"));
            return {};
        }

		struct {
			FString Path;
			FSoftObjectPath ReturnValue;
		}params;

		params.Path = Path;

		GetDefaultObj()->ProcessEvent(Function, &params);

		return params.ReturnValue;
	}

    RC::Unreal::UObject* UKismetSystemLibrary::LoadAsset_Blocking(UECustom::TSoftObjectPtr<UObject> Asset, bool bSetRootSet)
    {
        static auto Function = UECustom::UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, TEXT("/Script/Engine.KismetSystemLibrary:LoadAsset_Blocking"));

        if (!Function)
        {
            PS::Log<LogLevel::Error>(STR("Function /Script/Engine.KismetSystemLibrary:LoadAsset_Blocking was invalid.\n"));
            return nullptr;
        }

        struct {
            UECustom::TSoftObjectPtr<UObject> Asset;
            UObject* ReturnValue;
        }params;

        params.Asset = Asset;

        GetDefaultObj()->ProcessEvent(Function, &params);

        if (params.ReturnValue && bSetRootSet)
        {
            params.ReturnValue->SetRootSet();
        }

        return params.ReturnValue;
    }

    RC::Unreal::UObject* UKismetSystemLibrary::LoadAsset_Blocking(const RC::StringType& AssetPath, bool bSetRootSet)
    {
        auto SoftObjectPtr = UECustom::TSoftObjectPtr<UObject>(UECustom::FSoftObjectPath(AssetPath));
        auto LoadedAsset = LoadAsset_Blocking(SoftObjectPtr, bSetRootSet);
        return LoadedAsset;
    }

	UKismetSystemLibrary* UKismetSystemLibrary::GetDefaultObj()
	{
		static auto Self = UECustom::UObjectGlobals::StaticFindObject<UKismetSystemLibrary*>(nullptr, nullptr, TEXT("/Script/Engine.Default__KismetSystemLibrary"));
		return Self;
	}
}