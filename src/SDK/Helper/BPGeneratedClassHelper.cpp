#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/UPackage.hpp"
#include "SDK/Helper/BPGeneratedClassHelper.h"
#include "SDK/PalSignatures.h"
#include "SDK/Classes/Custom/UBlueprintGeneratedClass.h"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom::BPGeneratedClassHelper {
    UECustom::UBlueprintGeneratedClass* CreateInheritedBlueprintClass(RC::Unreal::UClass* parentClass, const RC::Unreal::FName& packageName,
        const RC::Unreal::FName& assetName, RC::Unreal::EObjectFlags objectFlags)
    {
        auto newPackage = UObjectGlobals::NewObject<UPackage>(nullptr, packageName, static_cast<EObjectFlags>(RF_Public));
        newPackage->SetRootSet();

        auto newBlueprintClass = UObjectGlobals::NewObject<UECustom::UBlueprintGeneratedClass>(newPackage, assetName, objectFlags);
        newBlueprintClass->PurgeClass(false);
        newBlueprintClass->GetClassFlags() |= CLASS_Transient;
        newBlueprintClass->GetClassWithin() = parentClass->GetClassWithin();
        newBlueprintClass->GetPropertyLink() = parentClass->GetPropertyLink();
        newBlueprintClass->SetSuperStruct(parentClass);
        newBlueprintClass->Bind();
        newBlueprintClass->StaticLink(true);
        newBlueprintClass->AssembleReferenceTokenStream(false);
        newBlueprintClass->SetRootSet();

        return newBlueprintClass;
    }

    UObject* UECustom::BPGeneratedClassHelper::FindComponentTemplateByName(UObject* BPGeneratedClass, FName TemplateName)
    {
        using FindComponentTemplateByNameSignature = UObject * (*)(UObject*, const FName&);
        static void* FunctionPtr = nullptr;

        if (!FunctionPtr)
        {
            FunctionPtr = Palworld::SignatureManager::GetSignature("UBlueprintGeneratedClass::FindComponentTemplateByName");
        }

        if (!FunctionPtr)
        {
            return nullptr;
        }

        return reinterpret_cast<FindComponentTemplateByNameSignature>(FunctionPtr)(BPGeneratedClass, TemplateName);
    }

    bool UECustom::BPGeneratedClassHelper::GetGeneratedClassesHierarchy(UClass* InClass, TArray<UObject*>& OutBPGClasses)
    {
        using GetGeneratedClassesHierarchySignature = bool(*)(UClass*, TArray<UObject*>&);
        static void* FunctionPtr = nullptr;

        if (!FunctionPtr)
        {
            FunctionPtr = Palworld::SignatureManager::GetSignature("UBlueprintGeneratedClass::FindComponentTemplateByName");
        }

        if (!FunctionPtr)
        {
            return false;
        }

        return reinterpret_cast<GetGeneratedClassesHierarchySignature>(FunctionPtr)(InClass, OutBPGClasses);
    }

}