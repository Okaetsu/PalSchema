#pragma once

#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "SDK/Classes/Custom/USimpleConstructionScript.h"

namespace RC::Unreal {
    class UObject;
}

namespace UECustom {
    class UInheritableComponentHandler;

    class UBlueprintGeneratedClass : public RC::Unreal::UClass {
    public:
        UInheritableComponentHandler* GetInheritableComponentHandler();

        USimpleConstructionScript* GetSimpleConstructionScript();
    };
}