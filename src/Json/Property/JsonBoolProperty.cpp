#include "Json/Property/JsonBoolProperty.h"

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

    void JsonBoolProperty::CopyValue(RC::Unreal::FProperty* Property, void* Container)
    {

    }

    void JsonBoolProperty::Print(RC::Unreal::FString& OutString, int Indent)
    {
        OutString = FString::Printf(TEXT("%s %s"), *OutString, InnerValue ? TEXT("true") : TEXT("false"));
    }

    bool JsonBoolProperty::Parse(const nlohmann::json& Data)
    {
        if (Data.is_boolean())
        {
            InnerValue = Data.get<bool>();
            return true;
        }

        return false;
    }
}