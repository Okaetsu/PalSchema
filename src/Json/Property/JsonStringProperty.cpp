#include "Json/Property/JsonStringProperty.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/CoreUObject/UObject/FStrProperty.hpp"
#include "Unreal/Property/FEnumProperty.hpp"
#include "Unreal/Property/FTextProperty.hpp"
#include "SDK/Helper/PropertyHelper.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonStringProperty::JsonStringProperty() : JsonProperty(JsonProperty::Type::String)
    {
    }

    JsonStringProperty::JsonStringProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::String)
    {
    }

    const FString& JsonStringProperty::GetValue() const
    {
        return InnerValue;
    }

    bool JsonStringProperty::Parse(const nlohmann::ordered_json& Data)
    {
        if (Data.is_string())
        {
            std::string ParsedString = Data.get<std::string>();
            RC::StringType WideName = RC::to_generic_string(ParsedString);

            FString NewValue = FString(WideName.c_str());
            ParseResourceString(NewValue);

            InnerValue = NewValue;
            return true;
        }

        return false;
    }

    void JsonStringProperty::CopyValue(RC::Unreal::FProperty* Property, void* Container)
    {
        using namespace Palworld::PropertyHelper;

        if (FStrProperty* StrProperty = CastProperty<FStrProperty>(Property))
        {
            StrProperty->SetPropertyValue(Container, GetValue());
        }
        else if (FNameProperty* NameProperty = CastProperty<FNameProperty>(Property))
        {
            NameProperty->SetPropertyValue(Container, FName(*GetValue(), FNAME_Add));
        }
        else if (FTextProperty* TextProperty = CastProperty<FTextProperty>(Property))
        {
            TextProperty->SetPropertyValue(Container, FText(*GetValue()));
        }
        else if (FEnumProperty* EnumProperty = CastProperty<FEnumProperty>(Property))
        {
            CopyEnumValue(EnumProperty, Container);
        }
        else if (FObjectProperty* ObjectProperty = CastProperty<FObjectProperty>(Property))
        {
            CopyObjectValue(ObjectProperty, Container);
        }
        else if (FSoftObjectProperty* SoftObjectProperty = CastProperty<FSoftObjectProperty>(Property))
        {
            CopySoftObjectValue(SoftObjectProperty, Container);
        }
        else
        {
            throw std::runtime_error(RC::fmt("Unsupported Type '%S' for Property '%S'",
                *GetTypeString(), Property->GetName().c_str()));
        }
    }

    void JsonStringProperty::Print(RC::Unreal::FString& OutString, int Indent)
    {
        OutString += InnerValue;
    }

    void JsonStringProperty::ParseResourceString(RC::Unreal::FString& String)
    {
        const FString ResourcePrefix = FString(TEXT("$resource/"));
        if (!String.StartsWith(ResourcePrefix))
        {
            return;
        }

        String.ReplaceInline(*ResourcePrefix, TEXT(""));
        String = FString::Printf(TEXT("/Engine/Transient.PalSchema/Resources/%s"), *String);
    }

    void JsonStringProperty::CopyObjectValue(FObjectProperty* ObjectProperty, void* Container)
    {
        auto SoftObjectPtr = FSoftObjectPtr(FSoftObjectPath(GetValue()));
        UObject* Asset = SoftObjectPtr.Get();
        if (!Asset)
        {
            throw std::runtime_error(RC::fmt("Failed to copy values over to '%S'. Asset was invalid.", ObjectProperty->GetName().c_str()));
        }

        UClass* ExpectedClass = ObjectProperty->GetPropertyClass();
        if (!Asset->IsA(ExpectedClass))
        {
            throw std::runtime_error(RC::fmt(
                "Failed to copy values over to '%S'. Asset didn't match the expected class for this property. Expected '%S', got '%S'",
                ObjectProperty->GetName().c_str(),
                ExpectedClass->GetName().c_str(),
                Asset->GetClassPrivate()->GetName().c_str())
            );
        }

        ObjectProperty->SetPropertyValue(Container, Asset);
    }

    void JsonStringProperty::CopySoftObjectValue(FSoftObjectProperty* SoftObjectProperty, void* Container)
    {
        auto SoftObjectPtr = FSoftObjectPtr(FSoftObjectPath(GetValue()));
        SoftObjectProperty->SetPropertyValue(Container, SoftObjectPtr);
    }

    void JsonStringProperty::CopyEnumValue(FEnumProperty* EnumProperty, void* Container)
    {
        UEnum* Enum = EnumProperty->GetEnum();
        if (!Enum)
        {
            throw std::runtime_error(RC::fmt("EnumProperty %S had a null Enum", EnumProperty->GetName().c_str()));
        }

        FString EnumString = InnerValue;
        if (!EnumString.Contains(TEXT("::")))
        {
            EnumString = FString::Printf(TEXT("%S::%S"), *EnumProperty->GetCPPType(), *InnerValue);
        }

        FName EnumName = FName(*EnumString);

        bool WasEnumFound = false;
        int64 EnumValue = 0;

        auto Names = Enum->GetEnumNames();
        const int32 TotalNames = Names.Num();
        for (int32 Index = 0; Index < TotalNames; ++Index)
        {
            if (Names[Index].Key.IsEqual(EnumName, ENameCase::IgnoreCase))
            {
                WasEnumFound = true;
                EnumValue = Names[Index].Value;
                break;
            }
        }

        if (!WasEnumFound)
        {
            throw std::runtime_error(RC::fmt("Enum value '%S' doesn't exist in %S", *EnumString, *EnumProperty->GetCPPType()));
        }

        FNumericProperty* UnderlyingProp = EnumProperty->GetUnderlyingProp();
        UnderlyingProp->SetIntPropertyValue(Container, EnumValue);
    }
}