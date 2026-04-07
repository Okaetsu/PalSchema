#include "SDK/Classes/Custom/UBlueprintGeneratedClass.h"
#include "SDK/Classes/Custom/UInheritableComponentHandler.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "SDK/Helper/Memory.h"
#include "SDK/Helper/PropertyHelper.h"
#include "SDK/PalSignatures.h"
#include "Utility/Logging.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"

using namespace Palworld;
using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    RC::Unreal::UClass* UBlueprintGeneratedClass::StaticClass()
    {
        static auto Class = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, TEXT("/Script/Engine.BlueprintGeneratedClass"));
        return Class;
    }

    void UBlueprintGeneratedClass::PurgeClass(bool bRecompilingOnLoad)
    {
        using FnSignature = void(*)(UClass*, bool);
        static FnSignature fn = nullptr;

        if (!fn)
        {
            auto staticClass = StaticClass();
            auto fnAddress = Palworld::GetVirtualFunctionFromClass(staticClass, 120);
            fn = static_cast<FnSignature>(fnAddress);
        }

        if (!fn)
        {
            PS::Log<LogLevel::Error>(STR("Failed to call UBlueprintGeneratedClass::PurgeClass because function address was invalid.\n"));
            return;
        }

        fn(this, bRecompilingOnLoad);
    }

    UInheritableComponentHandler* UBlueprintGeneratedClass::GetInheritableComponentHandler()
    {
        auto InheritableComponentHandler = 
            PropertyHelper::GetValuePtrByPropertyNameInChain<RC::Unreal::TObjectPtr<UInheritableComponentHandler>>(this, (STR("InheritableComponentHandler")));

        if (!InheritableComponentHandler)
        {
            return nullptr;
        }

        return (*InheritableComponentHandler).Get();
    }

    USimpleConstructionScript* UBlueprintGeneratedClass::GetSimpleConstructionScript()
    {
        auto SimpleConstructionScript =
            PropertyHelper::GetValuePtrByPropertyNameInChain<RC::Unreal::TObjectPtr<USimpleConstructionScript>>(this, (STR("SimpleConstructionScript")));

        if (!SimpleConstructionScript)
        {
            return nullptr;
        }

        return (*SimpleConstructionScript).Get();
    }
}