#include "Json/Property/JsonBoolProperty.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonBoolProperty::JsonBoolProperty() : JsonProperty(JsonProperty::Type::Bool)
    {
    }

    JsonBoolProperty::JsonBoolProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::Bool)
    {
    }

    const bool& JsonBoolProperty::GetValue() const
    {
        return InnerValue;
    }

    bool JsonBoolProperty::Parse(const nlohmann::ordered_json& Data)
    {
        if (Data.is_boolean())
        {
            InnerValue = Data.get<bool>();
            return true;
        }

        return false;
    }

    void JsonBoolProperty::CopyValue(FProperty* Property, void* Container)
    {
        using namespace Palworld::PropertyHelper;

        if (FBoolProperty* BoolProperty = CastProperty<FBoolProperty>(Property))
        {
            BoolProperty->SetPropertyValue(Container, GetValue());
        }
        else
        {
            throw std::runtime_error(RC::fmt("Unsupported Type '%S' for Property '%S'",
                *GetTypeString(), Property->GetName().c_str()));
        }
    }

    void JsonBoolProperty::Print(FString& OutString, int Indent)
    {
        OutString = FString::Printf(TEXT("%s %s"), *OutString, InnerValue ? TEXT("true") : TEXT("false"));
    }

    JsonProperty::Type JsonBoolProperty::StaticType()
    {
        return JsonProperty::Type::Bool;
    }
}