#include "Json/Property/JsonObjectProperty.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonObjectProperty::JsonObjectProperty() : JsonProperty(JsonProperty::Type::Object)
    {
    }

    JsonObjectProperty::JsonObjectProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::Object)
    {
    }

    void JsonObjectProperty::CopyValue(RC::Unreal::FProperty* Property, void* Container)
    {

    }

    void JsonObjectProperty::AddProperty(std::unique_ptr<JsonProperty> NewProperty)
    {
        Map.Add(NewProperty->GetName(), std::move(NewProperty));
    }

    void JsonObjectProperty::ForEachProperty(const std::function<void(const RC::Unreal::FName&, JsonProperty*)>& Callback)
    {
        for (const auto& Pair : Map)
        {
            Callback(Pair.Key, Pair.Value.get());
        }
    }

    void JsonObjectProperty::Print(RC::Unreal::FString& OutString, int Indent)
    {
        Indent++;
        for (const TPair<FName, std::unique_ptr<JsonProperty>>& Pair : Map)
        {
            OutString += TEXT("\n");
            PrintIndents(OutString, Indent);
            OutString += Pair.Key.ToFString() += TEXT(": ");
            Pair.Value->Print(OutString, Indent);
        }
    }

    bool JsonObjectProperty::Parse(const nlohmann::json& Data)
    {
        if (!Data.is_object())
        {
            return false;
        }

        for (const auto& [Key, Value] : Data.items())
        {
            std::unique_ptr<JsonProperty> NewProperty = CreateProperty(Key, Value);
            AddProperty(std::move(NewProperty));
        }

        return true;
    }
}