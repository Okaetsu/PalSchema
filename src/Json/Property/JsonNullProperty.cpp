#include "Json/Property/JsonNullProperty.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonNullProperty::JsonNullProperty() : JsonProperty(JsonProperty::Type::Bool)
    {
    }

    JsonNullProperty::JsonNullProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::Bool)
    {
    }

    bool JsonNullProperty::Parse(const nlohmann::ordered_json& Data)
    {
        if (Data.is_null())
        {
            return true;
        }

        return false;
    }

    void JsonNullProperty::CopyValue(FProperty* Property, void* Container)
    {
    }

    void JsonNullProperty::Print(FString& OutString, int Indent)
    {
        OutString += TEXT("NULL");
    }

    JsonProperty::Type JsonNullProperty::StaticType()
    {
        return JsonProperty::Type::Null;
    }
}