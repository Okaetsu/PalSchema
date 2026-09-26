#include "Json/Property/JsonStringProperty.h"

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

    void JsonStringProperty::CopyValue(RC::Unreal::FProperty* Property, void* Container)
    {

    }

    void JsonStringProperty::Print(RC::Unreal::FString& OutString, int Indent)
    {
        OutString += InnerValue;
    }

    bool JsonStringProperty::Parse(const nlohmann::json& Data)
    {
        if (Data.is_string())
        {
            std::string ParsedString = Data.get<std::string>();
            RC::StringType WideName = RC::to_generic_string(ParsedString);
            InnerValue = FString(WideName.c_str());
            return true;
        }

        return false;
    }
}