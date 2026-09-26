#include "Json/Property/JsonArrayProperty.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonArrayProperty::JsonArrayProperty() : JsonProperty(JsonProperty::Type::Array)
    {
    }

    JsonArrayProperty::JsonArrayProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::Array)
    {
    }

    void JsonArrayProperty::CopyValue(RC::Unreal::FProperty* Property, void* Container)
    {
        
    }

    EArrayOperationMode JsonArrayProperty::GetArrayOperationMode() const
    {
        return ArrayOperationMode;
    }

    void JsonArrayProperty::AddProperty(std::unique_ptr<JsonProperty> NewProperty)
    {
        Items.Add(std::move(NewProperty));
    }

    void JsonArrayProperty::ForEachProperty(const std::function<void(JsonProperty*)>& Callback)
    {
        for (const auto& Item : Items)
        {
            Callback(Item.get());
        }
    }

    void JsonArrayProperty::Print(RC::Unreal::FString& OutString, int Indent)
    {
        Indent++;
        for (auto& Item : Items)
        {
            OutString += TEXT("\n");
            PrintIndents(OutString, Indent);
            Item->Print(OutString, Indent);
        }
    }

    void JsonArrayProperty::ParseAsArray(const nlohmann::json& Data)
    {
        for (const nlohmann::json& Item : Data)
        {
            std::unique_ptr<JsonProperty> NewProperty = CreateProperty(Item);
            AddProperty(std::move(NewProperty));
        }
    }

    void JsonArrayProperty::ParseAsObject(const nlohmann::json& Data)
    {
        if (!Data.at("Items").is_array())
        {
            throw std::runtime_error(std::format("Field 'Items' must be an array."));
        }

        if (Data.contains("Action"))
        {
            if (!Data.at("Action").is_string())
            {
                throw std::runtime_error(std::format("Field 'Action' must be a string."));
            }

            auto Action = Data.at("Action").get<std::string>();
            if (Action == "Clear")
            {
                ArrayOperationMode = EArrayOperationMode::Replace;
            }
            else
            {
                ArrayOperationMode = EArrayOperationMode::Append;
            }
        }

        auto Items = Data.at("Items").get<nlohmann::json::array_t>();
        ParseAsArray(Items);
    }

    bool JsonArrayProperty::Parse(const nlohmann::json& Data)
    {
        if (Data.is_array())
        {
            ParseAsArray(Data);
        }
        else if (Data.is_object() && Data.contains("Items"))
        {
            ParseAsObject(Data);
        }

        return false;
    }
}