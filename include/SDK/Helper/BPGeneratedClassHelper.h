#pragma once

#include "Unreal/NameTypes.hpp"

namespace RC::Unreal {
    class UObject;
}

namespace UECustom {
    class UBlueprintGeneratedClass;
}

namespace UECustom::BPGeneratedClassHelper {
    UECustom::UBlueprintGeneratedClass* CreateInheritedBlueprintClass(RC::Unreal::UClass* parentClass, const RC::Unreal::FName& packageName,
                                                                     const RC::Unreal::FName& assetName, RC::Unreal::EObjectFlags objectFlags);

    RC::Unreal::UObject* FindComponentTemplateByName(RC::Unreal::UObject* BPGeneratedClass, RC::Unreal::FName TemplateName);

    bool GetGeneratedClassesHierarchy(RC::Unreal::UClass* InClass, RC::Unreal::TArray<RC::Unreal::UObject*>& OutBPGClasses);
}