#include "SDK/Classes/Custom/UBlueprintGeneratedClass.h"
#include "SDK/Classes/Custom/UInheritableComponentHandler.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "SDK/Classes/Custom/UObjectWrapper.h"
#include "SDK/Helper/PropertyHelper.h"

using namespace Palworld;

namespace UECustom {
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