#include "SDK/StaticClassStorage.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Unreal/CoreUObject/UObject/Class.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    void StaticClassStorage::Initialize()
    {
        auto NameToFieldClassMap = Palworld::PropertyHelper::GetNameToFieldClassMap();
        for (auto& [TypeName, StaticClass] : *NameToFieldClassMap)
        {
            if (TypeName == FName(STR("ObjectProperty")))
            {
                ObjectPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("ObjectPtrProperty")))
            {
                ObjectPtrPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("NameProperty")))
            {
                NamePropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("BoolProperty")))
            {
                BoolPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("ArrayProperty")))
            {
                ArrayPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("MapProperty")))
            {
                MapPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("StructProperty")))
            {
                StructPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("ClassProperty")))
            {
                ClassPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("ClassPtrProperty")))
            {
                ClassPtrPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("SoftClassProperty")))
            {
                SoftClassPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("SoftObjectProperty")))
            {
                SoftObjectPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("EnumProperty")))
            {
                EnumPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("TextProperty")))
            {
                TextPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("StrProperty")))
            {
                StrPropertyStaticClass = StaticClass;
            }
            if (TypeName == FName(STR("NumericProperty")))
            {
                NumericPropertyStaticClass = StaticClass;
            }

            PS::Log<LogLevel::Verbose>(STR("Found {}::StaticClass"), TypeName.ToString());
        }
    }
}