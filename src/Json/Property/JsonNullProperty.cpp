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

    void JsonNullProperty::CopyValue(RC::Unreal::FProperty* Property, void* Container)
    {

    }

    void JsonNullProperty::Print(RC::Unreal::FString& OutString, int Indent)
    {
        OutString += TEXT("NULL");
    }

    bool JsonNullProperty::Parse(const nlohmann::json& Data)
    {
        if (Data.is_null())
        {
            return true;
        }

        return false;
    }
}