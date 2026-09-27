#include "Json/Property/JsonArrayProperty.h"
#include "Json/Property/JsonBoolProperty.h"
#include "Json/Property/JsonFloatProperty.h"
#include "Json/Property/JsonIntProperty.h"
#include "Json/Property/JsonNullProperty.h"
#include "Json/Property/JsonObjectProperty.h"
#include "Json/Property/JsonStringProperty.h"
#include "Json/Property/JsonProperty.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonProperty::JsonProperty(const JsonProperty::Type& Type)
    {
        PropertyType = Type;
    }

    JsonProperty::JsonProperty(const FString& InName, const JsonProperty::Type& Type)
    {
        Name = FName(*InName, FNAME_Add);
        PropertyType = Type;
    }

    const FName& JsonProperty::GetName() const
    {
        return Name;
    }

    JsonProperty::Type JsonProperty::GetType() const
    {
        return PropertyType;
    }

    FString JsonProperty::GetTypeString() const
    {
        switch (PropertyType)
        {
        case PS::JsonProperty::Type::String:
            return FString(TEXT("String"));
        case PS::JsonProperty::Type::Integer:
            return FString(TEXT("Integer"));
        case PS::JsonProperty::Type::Float:
            return FString(TEXT("Float"));
        case PS::JsonProperty::Type::Bool:
            return FString(TEXT("Bool"));
        case PS::JsonProperty::Type::Null:
            return FString(TEXT("Null"));
        case PS::JsonProperty::Type::Object:
            return FString(TEXT("Object"));
        case PS::JsonProperty::Type::Array:
            return FString(TEXT("Array"));
        }

        return FString(TEXT("Undefined"));
    }

    void JsonProperty::Dump(FString& OutString)
    {
        Print(OutString, 0);
    }

    void JsonProperty::PrintIndents(FString& OutString, int Indent)
    {
        for (int i = 0; i < Indent; i++)
        {
            OutString += TEXT("\t");
        }
    }

    std::unique_ptr<JsonProperty> JsonProperty::CreateProperty(const nlohmann::json& Data)
    {
        FString PropertyName{};
        std::unique_ptr<JsonProperty> Property = CreateProperty(PropertyName, Data);
        return std::move(Property);
    }

    std::unique_ptr<JsonProperty> JsonProperty::CreateProperty(const std::string& InName, const nlohmann::json& Data)
    {
        RC::StringType WideName = RC::to_generic_string(InName);
        FString PropertyName = FString(WideName.c_str());

        std::unique_ptr<JsonProperty> Property = CreateProperty(PropertyName, Data);
        return std::move(Property);
    }

    std::unique_ptr<JsonProperty> JsonProperty::CreateProperty(const FString& InName, const nlohmann::json& Data)
    {
        std::unique_ptr<JsonProperty> Property = nullptr;

        if (Data.is_number_integer())
        {
            Property = std::make_unique<JsonIntProperty>(InName);
        }
        else if (Data.is_number_float())
        {
            Property = std::make_unique<JsonFloatProperty>(InName);
        }
        else if (Data.is_array() || (Data.is_object() && Data.contains("Items")))
        {
            Property = std::make_unique<JsonArrayProperty>(InName);
        }
        else if (Data.is_boolean())
        {
            Property = std::make_unique<JsonBoolProperty>(InName);
        }
        else if (Data.is_string())
        {
            Property = std::make_unique<JsonStringProperty>(InName);
        }
        else if (Data.is_object())
        {
            Property = std::make_unique<JsonObjectProperty>(InName);
        }
        else if (Data.is_null())
        {
            Property = std::make_unique<JsonNullProperty>(InName);
        }

        if (Property)
        {
            Property->Parse(Data);
        }

        return std::move(Property);
    }
}