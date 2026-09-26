#include "Json/Property/JsonIntProperty.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonIntProperty::JsonIntProperty() : JsonProperty(JsonProperty::Type::Integer)
    {
    }

    JsonIntProperty::JsonIntProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::Integer)
    {
    }

    const RC::Unreal::int64& JsonIntProperty::GetValue() const
    {
        return InnerValue;
    }

    void JsonIntProperty::CopyValue(RC::Unreal::FProperty* Property, void* Container)
    {

    }

    void JsonIntProperty::Print(RC::Unreal::FString& OutString, int Indent)
    {
        OutString = FString::Printf(TEXT("%s %d"), *OutString, InnerValue);
    }

    bool JsonIntProperty::Parse(const nlohmann::json& Data)
    {
        if (Data.is_number_integer())
        {
            InnerValue = Data.get<int64>();
            return true;
        }

        return false;
    }
}