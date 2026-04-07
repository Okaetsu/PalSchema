#pragma once

#include "SDK/Classes/Custom/UClassWrapper.h"
#include "SDK/Classes/Custom/USimpleConstructionScript.h"

namespace RC::Unreal {
    class UObject;
}

namespace UECustom {
    class UInheritableComponentHandler;

    class UBlueprintGeneratedClass : public UECustom::UClassWrapper {
    public:
        static RC::Unreal::UClass* StaticClass();

        void PurgeClass(bool bRecompilingOnLoad);

        UInheritableComponentHandler* GetInheritableComponentHandler();

        USimpleConstructionScript* GetSimpleConstructionScript();
    };
}