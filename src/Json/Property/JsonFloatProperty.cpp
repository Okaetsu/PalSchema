#include "Json/Property/JsonFloatProperty.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonFloatProperty::JsonFloatProperty() : JsonProperty(JsonProperty::Type::Float)
    {
    }

    JsonFloatProperty::JsonFloatProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::Float)
    {
    }

    const double& JsonFloatProperty::GetValue() const
    {
        return InnerValue;
    }

    void JsonFloatProperty::CopyValue(RC::Unreal::FProperty* Property, void* Container)
    {

    }

    void JsonFloatProperty::Print(RC::Unreal::FString& OutString, int Indent)
    {
        OutString = FString::Printf(TEXT("%s %f"), *OutString, InnerValue);
    }

    bool JsonFloatProperty::Parse(const nlohmann::json& Data)
    {
        if (Data.is_number_float())
        {
            InnerValue = Data.get<double>();
            return true;
        }

        return false;
    }
}