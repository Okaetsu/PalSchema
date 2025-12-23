#pragma once

#include "SDK/Classes/Custom/UObjectWrapper.h"
#include "Unreal/UnrealCoreStructs.hpp"

namespace UECustom {
    struct FComponentKey
    {
        RC::Unreal::TObjectPtr<RC::Unreal::UClass> OwnerClass;

        RC::Unreal::FName SCSVariableName;

        RC::Unreal::FGuid AssociatedGuid;
    };

    struct FComponentOverrideRecord
    {
        RC::Unreal::TObjectPtr<RC::Unreal::UClass> ComponentClass;

        RC::Unreal::TObjectPtr<RC::Unreal::UObject> ComponentTemplate;

        FComponentKey ComponentKey;

        RC::Unreal::uint8 CookedComponentInstancingData[0x48];
    };

    class UInheritableComponentHandler : public UECustom::UObjectWrapper {
    public:
        RC::Unreal::TArray<FComponentOverrideRecord> GetRecords();
    };
}