#include "Unreal/FProperty.hpp"
#include "Unreal/Property/FArrayProperty.hpp"
#include "Unreal/Property/FBoolProperty.hpp"
#include "Unreal/Property/FNameProperty.hpp"
#include "Unreal/Property/FNumericProperty.hpp"
#include "Unreal/Property/FEnumProperty.hpp"
#include "Unreal/Property/FMapProperty.hpp"
#include "Unreal/Property/FStructProperty.hpp"
#include "Unreal/Property/FStrProperty.hpp"
#include "Unreal/Property/FClassProperty.hpp"
#include "Unreal/Property/FSoftClassProperty.hpp"
#include "Unreal/Property/FSoftObjectProperty.hpp"
#include "Unreal/Property/FTextProperty.hpp"
#include "Unreal/FString.hpp"
#include "Unreal/NameTypes.hpp"
#include "Unreal/UClass.hpp"
#include "Unreal/UEnum.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "SDK/Classes/TSoftObjectPtr.h"
#include "SDK/Classes/TSoftClassPtr.h"
#include "SDK/Classes/KismetSystemLibrary.h"
#include "SDK/Structs/Custom/FManagedValue.h"
#include "SDK/Structs/Custom/FScriptMapHelper.h"
#include "SDK/Structs/Custom/FScriptArrayHelper.h"
#include "Utility/Logging.h"
#include "SDK/Helper/PropertyHelper.h"
#include "SDK/PalSignatures.h"
#include "Helpers/Casting.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    void PropertyHelper::CopyJsonValueToContainer(void* Container, FProperty* Property, const nlohmann::json& Value)
    {
        auto PropertyName = Property->GetName();
        auto Type = Property->GetCPPType();
        auto Class = Property->GetClass();
        auto ClassName = Class.GetName();
        auto ValuePtr = Property->ContainerPtrToValuePtr<void>(Container);

        if (auto EnumProperty = CastProperty<FEnumProperty>(Property))
        {
            SetEnumPropertyValueFromJsonValue(ValuePtr, EnumProperty, Value);
        }
        else if (auto NumProperty = CastProperty<FNumericProperty>(Property))
        {
            SetNumericPropertyValueFromJsonValue(ValuePtr, NumProperty, Value);
        }
        else if (auto BoolProperty = CastProperty<FBoolProperty>(Property))
        {
            SetBoolPropertyValueFromJsonValue(ValuePtr, BoolProperty, Value);
        }
        else if (auto NameProperty = CastProperty<FNameProperty>(Property))
        {
            SetNamePropertyValueFromJsonValue(ValuePtr, NameProperty, Value);
        }
        else if (auto StrProperty = CastProperty<FStrProperty>(Property))
        {
            SetStrPropertyValueFromJsonValue(ValuePtr, StrProperty, Value);
        }
        else if (auto TextProperty = CastProperty<FTextProperty>(Property))
        {
            SetTextPropertyValueFromJsonValue(ValuePtr, TextProperty, Value);
        }
        else if (auto ClassProperty = CastProperty<FClassProperty>(Property))
        {
            SetClassPropertyValueFromJsonValue(ValuePtr, ClassProperty, Value);
        }
        else if (CastProperty<FObjectProperty>(Property) && ClassName == STR("ObjectProperty"))
        {
            auto ObjectProperty = CastProperty<FObjectProperty>(Property);
            SetObjectPropertyValueFromJsonValue(Container, ObjectProperty, Value);
        }
        else if (CastProperty<FSoftObjectProperty>(Property) && ClassName == STR("SoftObjectProperty"))
        {
            auto SoftObjectProperty = CastProperty<FSoftObjectProperty>(Property);
            SetSoftObjectPropertyValueFromJsonValue(ValuePtr, SoftObjectProperty, Value);
        }
        else if (CastProperty<FSoftClassProperty>(Property) && ClassName == STR("SoftClassProperty"))
        {
            auto SoftClassProperty = CastProperty<FSoftClassProperty>(Property);
            SetSoftClassPropertyValueFromJsonValue(ValuePtr, SoftClassProperty, Value);
        }
        else if (auto StructProperty = CastProperty<FStructProperty>(Property))
        {
            SetStructPropertyValueFromJsonValue(ValuePtr, StructProperty, Value);
        }
        else if (auto ArrayProperty = CastProperty<FArrayProperty>(Property))
        {
            SetArrayPropertyValueFromJsonValue(ValuePtr, ArrayProperty, Value);
        }
        else if (auto MapProperty = CastProperty<FMapProperty>(Property))
        {
            SetMapPropertyValueFromJsonValue(ValuePtr, MapProperty, Value);
        }
        else
        {
            PS::Log<RC::LogLevel::Warning>(STR("Unhandled property '{}' with class of {} and type of {}\n"), PropertyName, ClassName, Type.GetCharArray());
        }
    }

    int64 PropertyHelper::ParseEnumFromJsonValue(FEnumProperty* Property, const nlohmann::json& Value)
    {
        auto PropertyName = GetPropertyNameAsUTF8String(Property);
        auto PropertyType = GetPropertyTypeAsUTF8String(Property);

        ValidateJsonValueType(Property, Value);

        auto Enum = Property->GetEnum();
        if (!Enum)
        {
            throw std::runtime_error(std::format("EnumProperty {} had an invalid Enum value", PropertyName));
        }

        auto ParsedValue = Value.get<std::string>();
        if (!ParsedValue.contains("::"))
        {
            ParsedValue = std::format("{}::{}", PropertyType, ParsedValue);
        }

        auto EnumName = FName(RC::to_generic_string(ParsedValue));

        bool WasEnumFound = false;
        int64_t EnumValue = 0;

        for (const auto& EnumPair : Enum->GetNames())
        {
            if (EnumPair.Key == EnumName)
            {
                WasEnumFound = true;
                EnumValue = EnumPair.Value;
            }
        }

        if (!WasEnumFound)
        {
            throw std::runtime_error(std::format("Enum '{}' doesn't exist", ParsedValue));
        }

        return EnumValue;
    }

    int64 PropertyHelper::ParseByteFromJsonValue(FNumericProperty* Property, const nlohmann::json& Value)
    {
        auto PropertyName = GetPropertyNameAsUTF8String(Property);
        auto PropertyType = GetPropertyTypeAsUTF8String(Property);

        auto Enum = Property->GetIntPropertyEnum();
        if (!Enum)
        {
            throw std::runtime_error(std::format("EnumProperty {} had an invalid Enum value", PropertyName));
        }

        auto ParsedValue = Value.get<std::string>();
        if (!ParsedValue.contains("::"))
        {
            ParsedValue = std::format("{}::{}", PropertyType, ParsedValue);
        }

        auto EnumName = FName(RC::to_generic_string(ParsedValue));

        bool WasEnumFound = false;
        int64_t EnumValue = 0;

        for (const auto& EnumPair : Enum->GetNames())
        {
            if (EnumPair.Key == EnumName)
            {
                WasEnumFound = true;
                EnumValue = EnumPair.Value;
            }
        }

        if (!WasEnumFound)
        {
            throw std::runtime_error(std::format("Enum '{}' doesn't exist", ParsedValue));
        }

        return EnumValue;
    }

    void PropertyHelper::SetEnumPropertyValueFromJsonValue(void* Data, FEnumProperty* Property, const nlohmann::json& Value)
    {
        auto EnumValue = ParseEnumFromJsonValue(Property, Value);
        FMemory::Memcpy(Data, &EnumValue, Property->GetElementSize());
    }

    void PropertyHelper::SetNumericPropertyValueFromJsonValue(void* Data, RC::Unreal::FNumericProperty* Property, const nlohmann::json& Value)
    {
        auto PropertyName = GetPropertyNameAsUTF8String(Property);
        if (!Property->IsEnum())
        {
            ValidateJsonValueType(Property, Value);
        }

        if (Property->IsEnum())
        {
            auto EnumValue = ParseByteFromJsonValue(Property, Value);
            Property->SetIntPropertyValue(Data, EnumValue);
        }
        else
        {
            if (Property->IsInteger())
            {
                Property->SetIntPropertyValue(Data, Value.get<int64>());
            }
            else if (Property->IsFloatingPoint())
            {
                Property->SetFloatingPointPropertyValue(Data, Value.get<double>());
            }
            else
            {
                PS::Log<RC::LogLevel::Warning>(STR("Unhandled Numeric Type: {}\n"), Property->GetName());
            }
        }
    }

    void PropertyHelper::SetBoolPropertyValueFromJsonValue(void* Data, RC::Unreal::FBoolProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        Property->SetPropertyValue(Data, Value.get<bool>());
    }

    void PropertyHelper::SetNamePropertyValueFromJsonValue(void* Data, RC::Unreal::FNameProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto ParsedValue = Value.get<std::string>();
        auto Name = FName(RC::to_generic_string(ParsedValue), FNAME_Add);
        Property->SetPropertyValue(Data, Name);
    }

    void PropertyHelper::SetStrPropertyValueFromJsonValue(void* Data, RC::Unreal::FStrProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto ParsedValue = Value.get<std::string>();
        auto String = FString(RC::to_generic_string(ParsedValue).c_str());
        Property->SetPropertyValue(Data, String);
    }

    void PropertyHelper::SetTextPropertyValueFromJsonValue(void* Data, RC::Unreal::FTextProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto StringValue = Value.get<std::string>();
        auto Text = FText(RC::to_generic_string(StringValue).c_str());
        Property->SetPropertyValue(Data, Text);
    }

    void PropertyHelper::SetClassPropertyValueFromJsonValue(void* Data, RC::Unreal::FClassProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto PropertyName = GetPropertyNameAsUTF8String(Property);

        auto StringValue = Value.get<std::string>();
        auto StringValueWide = RC::to_generic_string(StringValue);
        auto SoftObjectPtr = UECustom::TSoftObjectPtr<UObject>(UECustom::FSoftObjectPath(StringValueWide));
        auto Asset = UECustom::UKismetSystemLibrary::LoadAsset_Blocking(SoftObjectPtr);

        if (!Asset)
        {
            throw std::runtime_error(std::format("Property {} was supplied an invalid class of {}", PropertyName, StringValue));
        }

        Property->SetPropertyValue(Data, Asset);
    }

    void PropertyHelper::SetObjectPropertyValueFromJsonValue(void* Data, RC::Unreal::FObjectProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto ParsedValue = Value.get<nlohmann::json>();
        auto ObjectValue = *Property->ContainerPtrToValuePtr<UObject*>(Data);
        if (ObjectValue)
        {
            for (auto& [InnerKey, InnerValue] : Value.items())
            {
                auto ObjectValue_PropertyName = RC::to_generic_string(InnerKey);
                auto ObjectValue_Property = ObjectValue->GetPropertyByNameInChain(ObjectValue_PropertyName.c_str());

                if (!ObjectValue_Property)
                {
                    ObjectValue_Property = Palworld::PropertyHelper::GetPropertyByName(ObjectValue->GetClassPrivate(), ObjectValue_PropertyName);
                }

                if (ObjectValue_Property)
                {
                    CopyJsonValueToContainer(ObjectValue, ObjectValue_Property, InnerValue);
                }
            }
        }
    }

    void PropertyHelper::SetSoftClassPropertyValueFromJsonValue(void* Data, RC::Unreal::FSoftClassProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto ParsedValue = Value.get<std::string>();
        auto String = RC::to_generic_string(ParsedValue);
        if (!String.ends_with(STR("_C"))) String += STR("_C");

        auto SoftClassPtr = UECustom::TSoftClassPtr<UClass>(UECustom::FSoftObjectPath(String));
        FMemory::Memcpy(Data, &SoftClassPtr, sizeof(UECustom::TSoftClassPtr<UClass>));
    }

    void PropertyHelper::SetSoftObjectPropertyValueFromJsonValue(void* Data, RC::Unreal::FSoftObjectProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);
        const std::string resourcePrefix = "$resource/";

        auto ParsedValue = Value.get<std::string>();

        RC::StringType PackagePath = RC::to_generic_string(ParsedValue);

        if (ParsedValue.starts_with(resourcePrefix))
        {
            // Before: "$resource/modname/resourcename"
            // After:  "modname/resourcename"
            PackagePath = PackagePath.erase(0, resourcePrefix.length());

            // "/Engine/Transient.PalSchema/Resources/modname/resourcename"
            PackagePath = std::format(TEXT("/Engine/Transient.PalSchema/Resources/{}"), PackagePath);
        }

        auto SoftObjectPtr = UECustom::TSoftObjectPtr<UObject>(UECustom::FSoftObjectPath(PackagePath));
        FMemory::Memcpy(Data, &SoftObjectPtr, sizeof(UECustom::TSoftObjectPtr<UObject>));
    }

    void PropertyHelper::SetStructPropertyValueFromJsonValue(void* Data, RC::Unreal::FStructProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto ParsedObject = Value.get<nlohmann::json>();

        auto Struct = Property->GetStruct();
        if (!Struct)
        {
            throw std::runtime_error(std::format("Failed to get Struct"));
        }

        FField* Field = Struct->GetChildProperties();
        while (Field)
        {
            auto FieldName = GetPropertyNameAsUTF8String(static_cast<FProperty*>(Field));
            if (Value.contains(FieldName))
            {
                CopyJsonValueToContainer(Data, static_cast<FProperty*>(Field), Value.at(FieldName));
            }

            Field = GetNextField(Field);
        }
    }

    void PropertyHelper::SetArrayPropertyValueFromJsonValue(void* Data, RC::Unreal::FArrayProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto ParsedValue = Value.get<nlohmann::json>();

        auto ScriptArray = static_cast<FScriptArray*>(Data);
        auto ScriptArrayHelper = UECustom::FScriptArrayHelper(ScriptArray, Property);

        auto InnerProperty = Property->GetInner();

        if (Value.is_object())
        {
            if (Value.contains("Action"))
            {
                auto Action = Value.at("Action").get<std::string>();
                if (Action == "Clear")
                {
                    ScriptArrayHelper.Empty();
                }
            }

            if (Value.contains("Items"))
            {
                if (!Value.at("Items").is_array())
                {
                    throw std::runtime_error(std::format("Field Items must be an array"));
                }

                auto Items = Value.at("Items").get<nlohmann::json::array_t>();
                for (auto& Item : Items)
                {
                    UECustom::FManagedValue ValuePtr;
                    ScriptArrayHelper.InitializeValue(ValuePtr);
                    CopyJsonValueToContainer(ValuePtr.GetData(), InnerProperty, Item);
                    ScriptArrayHelper.Add(ValuePtr);
                }
            }
        }
        else if (Value.is_array())
        {
            ScriptArrayHelper.Empty();

            auto Items = Value.get<nlohmann::json::array_t>();
            for (auto& Item : Items)
            {
                UECustom::FManagedValue ValuePtr;
                ScriptArrayHelper.InitializeValue(ValuePtr);
                CopyJsonValueToContainer(ValuePtr.GetData(), InnerProperty, Item);
                ScriptArrayHelper.Add(ValuePtr);
            }
        }
    }

    void PropertyHelper::SetMapPropertyValueFromJsonValue(void* Data, RC::Unreal::FMapProperty* Property, const nlohmann::json& Value)
    {
        ValidateJsonValueType(Property, Value);

        auto ArrayItems = Value.get<std::vector<nlohmann::json>>();

        auto KeyProperty = Property->GetKeyProp();
        auto ValueProperty = Property->GetValueProp();

        auto MapLayout = FScriptMap::GetScriptLayout(
            KeyProperty->GetSize(),
            KeyProperty->GetMinAlignment(),
            ValueProperty->GetSize(),
            ValueProperty->GetMinAlignment());

        auto ScriptMap = static_cast<Unreal::FScriptMap*>(Data);
        auto ScriptMapHelper = UECustom::FScriptMapHelper(ScriptMap, MapLayout, KeyProperty, ValueProperty);

        for (const auto& Entry : ArrayItems)
        {
            if (!Entry.contains("Key") || !Entry.contains("Value"))
            {
                throw std::runtime_error("Each TMap entry must have a 'Key' and 'Value' property.");
            }

            UECustom::FManagedValue ScopedPair;

            ScriptMapHelper.InitializePair(ScopedPair);

            CopyJsonValueToContainer(ScopedPair.GetData(), KeyProperty, Entry.at("Key"));
            CopyJsonValueToContainer(ScopedPair.GetData(), ValueProperty, Entry.at("Value"));

            ScriptMapHelper.Add(ScopedPair);
        }

        ScriptMap->Rehash(MapLayout,
        [&](const void* Src) -> uint32 {
            return KeyProperty->GetValueTypeHash(Src);
        });
    }

    void PropertyHelper::ValidateJsonValueType(RC::Unreal::FProperty* Property, const nlohmann::json& Value)
    {
        auto PropertyName = GetPropertyNameAsUTF8String(Property);
        auto PropertyClass = Property->GetClass();
        auto PropertyClassName = PropertyClass.GetName();

        if (auto EnumProperty = CastProperty<FEnumProperty>(Property))
        {
            if (!Value.is_string()) throw std::runtime_error(std::format("Property {} must be a string", PropertyName));
        }
        else if (auto NumProperty = CastProperty<FNumericProperty>(Property))
        {
            if (!Value.is_number()) throw std::runtime_error(std::format("Property {} must be a number", PropertyName));
        }
        else if (auto BoolProperty = CastProperty<FBoolProperty>(Property))
        {
            if (!Value.is_boolean()) throw std::runtime_error(std::format("Property {} must be a boolean", PropertyName));
        }
        else if (auto NameProperty = CastProperty<FNameProperty>(Property))
        {
            if (!Value.is_string()) throw std::runtime_error(std::format("Property {} must be a string", PropertyName));
        }
        else if (auto StrProperty = CastProperty<FStrProperty>(Property))
        {
            if (!Value.is_string()) throw std::runtime_error(std::format("Property {} must be a string", PropertyName));
        }
        else if (auto TextProperty = CastProperty<FTextProperty>(Property))
        {
            if (!Value.is_string()) throw std::runtime_error(std::format("Property {} must be a string", PropertyName));
        }
        else if (auto ClassProperty = CastProperty<FClassProperty>(Property))
        {
            if (!Value.is_string()) throw std::runtime_error(std::format("Property {} must be a string", PropertyName));
        }
        else if (auto ObjectProperty = CastProperty<FObjectProperty>(Property) && PropertyClassName == STR("ObjectProperty"))
        {
            if (!Value.is_object()) throw std::runtime_error(std::format("Property {} must be an object", PropertyName));
        }
        else if (auto SoftObjectProperty = CastProperty<FSoftObjectProperty>(Property) && PropertyClassName == STR("SoftObjectProperty"))
        {
            if (!Value.is_string()) throw std::runtime_error(std::format("Property {} must be a string", PropertyName));
        }
        else if (auto SoftClassProperty = CastProperty<FSoftClassProperty>(Property) && PropertyClassName == STR("SoftClassProperty"))
        {
            if (!Value.is_string()) throw std::runtime_error(std::format("Property {} must be a string", PropertyName));
        }
        else if (auto StructProperty = CastProperty<FStructProperty>(Property))
        {
            if (!Value.is_object()) throw std::runtime_error(std::format("Property {} must be an object", PropertyName));
        }
        else if (auto ArrayProperty = CastProperty<FArrayProperty>(Property))
        {
            if (!Value.is_object() && !Value.is_array()) throw std::runtime_error(std::format("Property {} must be an object or array", PropertyName));
        }
        else if (auto MapProperty = CastProperty<FMapProperty>(Property))
        {
            if (!Value.is_array()) throw std::runtime_error(std::format("Property {} must be an array of objects", PropertyName));
        }
    }

    std::string PropertyHelper::GetPropertyNameAsUTF8String(FProperty* Property)
    {
        auto PropertyName = Property->GetName();
        auto PropertyNameUTF8 = RC::to_string(PropertyName);
        return PropertyNameUTF8;
    }

    std::string PropertyHelper::GetPropertyTypeAsUTF8String(FProperty* Property)
    {
        auto PropertyType = RC::to_string(*Property->GetCPPType());
        return PropertyType;
    }

    RC::Unreal::FProperty* PropertyHelper::GetPropertyByName(RC::Unreal::UClass* Class, const RC::StringType& PropertyName)
    {
        FProperty* Property = nullptr;
        for (FProperty* It = Class->GetPropertyLink(); It != nullptr; It = It->GetPropertyLinkNext())
        {
            if (It->GetName() == PropertyName)
            {
                Property = It;
            }
        }
        return Property;
    }

    RC::Unreal::FProperty* PropertyHelper::GetPropertyByName(RC::Unreal::UScriptStruct* Struct, const RC::StringType& PropertyName)
    {
        FProperty* Property = nullptr;
        FName PropertyFName = FName(PropertyName, FNAME_Add);
        for (FProperty* It = Struct->GetPropertyLink(); It != nullptr; It = It->GetPropertyLinkNext())
        {
            if (It->GetFName() == PropertyFName)
            {
                Property = It;
            }
        }
        return Property;
    }

    RC::Unreal::FFieldClass* PropertyHelper::FindFieldClassByName(const RC::Unreal::FName& Name)
    {
        auto NameToFieldClassMap = GetNameToFieldClassMap();
        if (!NameToFieldClassMap)
        {
            return nullptr;
        }

        auto FieldClass = NameToFieldClassMap->Find(Name);
        if (!FieldClass)
        {
            return nullptr;
        }

        return *FieldClass;
    }

    FFieldClass* PropertyHelper::FindFieldClassByName(const RC::StringType& Name)
    {
        auto NewName = FName(Name, FNAME_Add);
        return FindFieldClassByName(NewName);
    }

    FField* PropertyHelper::GetNextField(FField* Field)
    {
        auto Next = *Helper::Casting::ptr_cast<FField**>(Field, 0x20);
        return Next;
    }

    TMap<FName, FFieldClass*>* PropertyHelper::GetNameToFieldClassMap()
    {
        using GetNameToFieldClassMap_Signature = TMap<FName, FFieldClass*>*(*)();
        static GetNameToFieldClassMap_Signature GetNameToFieldClassMap_Internal = nullptr;

        if (!GetNameToFieldClassMap_Internal)
        {
            GetNameToFieldClassMap_Internal = reinterpret_cast<GetNameToFieldClassMap_Signature>(
                Palworld::SignatureManager::GetSignature("FFieldClass::GetNameToFieldClassMap")
            );
        }

        if (!GetNameToFieldClassMap_Internal)
        {
            PS::Log<LogLevel::Error>(STR("Failed to call FFieldClass::GetNameToFieldClassMap because function address was invalid.\n"));
            return nullptr;
        }

        return GetNameToFieldClassMap_Internal();
    }

    bool PropertyHelper::IsPropertyA(RC::Unreal::FField* Field, RC::Unreal::FFieldClass* FieldClass)
    {
        using IsA_Signature = bool(*)(FField*, FFieldClass*);
        static IsA_Signature IsA_Internal = nullptr;

        if (!IsA_Internal)
        {
            IsA_Internal = reinterpret_cast<IsA_Signature>(
                Palworld::SignatureManager::GetSignature("FField::IsA")
            );
        }

        if (!IsA_Internal)
        {
            PS::Log<LogLevel::Error>(STR("Failed to call FField::IsA because function address was invalid.\n"));
            return false;
        }

        return IsA_Internal(Field, FieldClass);
    }
}