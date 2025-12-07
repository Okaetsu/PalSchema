#pragma once

#include "Unreal/UObject.hpp"

namespace UECustom {

    namespace UObjectGlobals {
        RC::Unreal::UObject* StaticFindObject(RC::Unreal::UClass* ObjectClass, RC::Unreal::UObject* InObjectPackage, const RC::Unreal::TCHAR* OrigInName = nullptr, bool bExactClass = false);

        template<RC::Unreal::UObjectPointerDerivative ObjectType = RC::Unreal::UObject*>
        ObjectType StaticFindObject(RC::Unreal::UClass* ObjectClass, RC::Unreal::UObject* InObjectPackage, const RC::Unreal::TCHAR* OrigInName = nullptr, bool bExactClass = false)
        {
            auto Object = StaticFindObject(ObjectClass, InObjectPackage, OrigInName, bExactClass);
            return static_cast<ObjectType>(Object);
        }

        void GetObjectsOfClass(const RC::Unreal::UClass* ClassToLookFor, RC::Unreal::TArray<RC::Unreal::UObject*>& Results, 
                               bool bIncludeDerivedClasses = true, RC::Unreal::EObjectFlags ExcludeFlags = RC::Unreal::RF_ClassDefaultObject, 
                               RC::Unreal::EInternalObjectFlags ExclusionInternalFlags = RC::Unreal::EInternalObjectFlags::None);
    }
}