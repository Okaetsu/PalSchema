#include "Json/Property/JsonArrayProperty.h"
#include "Json/Property/JsonObjectProperty.h"
#include "SDK/Helper/PropertyHelper.h"
#include "SDK/Structs/Custom/FScriptMapHelper.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"

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

    bool JsonArrayProperty::Parse(const nlohmann::ordered_json& Data)
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

    void JsonArrayProperty::CopyValue(FProperty* Property, void* Container)
    {
        using namespace Palworld::PropertyHelper;

        if (FArrayProperty* ArrayProperty = CastProperty<FArrayProperty>(Property))
        {
            CopyArrayValue(ArrayProperty, Container);
        }
        else if (FMapProperty* MapProperty = CastProperty<FMapProperty>(Property))
        {
            CopyMapValue(MapProperty, Container);
        }
        else
        {
            throw std::runtime_error(RC::fmt("Unsupported Type '%S' for Property '%S'",
                *GetTypeString(), Property->GetName().c_str()));
        }
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

    void JsonArrayProperty::Print(FString& OutString, int Indent)
    {
        Indent++;
        for (auto& Item : Items)
        {
            OutString += TEXT("\n");
            PrintIndents(OutString, Indent);
            Item->Print(OutString, Indent);
        }
    }

    void JsonArrayProperty::ParseAsArray(const nlohmann::ordered_json& Data)
    {
        for (const nlohmann::ordered_json& Item : Data)
        {
            std::unique_ptr<JsonProperty> NewProperty = CreateProperty(Item);
            AddProperty(std::move(NewProperty));
        }
    }

    void JsonArrayProperty::ParseAsObject(const nlohmann::ordered_json& Data)
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

        auto Items = Data.at("Items").get<nlohmann::ordered_json::array_t>();
        ParseAsArray(Items);
    }

    void JsonArrayProperty::CopyArrayValue(FArrayProperty* Property, void* Container)
    {
        FScriptArrayHelper ArrayHelper(Property, Container);
        FProperty* InnerProp = Property->GetInner();

        if (ArrayOperationMode == EArrayOperationMode::Replace)
        {
            ArrayHelper.EmptyValues(Items.Num());
        }

        for (std::unique_ptr<JsonProperty>& ItemProp : Items)
        {
            int32 NewIndex = ArrayHelper.AddValue();
            uint8* RawPtr = ArrayHelper.GetRawPtr(NewIndex);
            ItemProp->CopyValue(InnerProp, RawPtr);
        }
    }

    void JsonArrayProperty::CopyMapValue(FMapProperty* Property, void* Container)
    {
        FProperty* KeyProp = Property->GetKeyProp();
        FProperty* ValueProp = Property->GetValueProp();

        FScriptMapLayout& MapLayout = Property->GetMapLayout();
        FScriptMap* ScriptMap = static_cast<FScriptMap*>(Container);
        auto ScriptMapHelper = UECustom::FScriptMapHelper(ScriptMap, MapLayout, KeyProp, ValueProp);

        for (auto& ItemProp : Items)
        {
            if (ItemProp->GetType() != JsonProperty::Type::Object)
            {
                throw std::runtime_error(RC::fmt("Expected an Object value for %S", Property->GetName().c_str()));
            }

            JsonObjectProperty* ObjectItemProp = static_cast<JsonObjectProperty*>(ItemProp.get());
            JsonProperty* KeyItemProp = ObjectItemProp->GetPropertyByName(FName(TEXT("Key"), FNAME_Add));
            JsonProperty* ValueItemProp = ObjectItemProp->GetPropertyByName(FName(TEXT("Value"), FNAME_Add));

            if (!KeyItemProp || !ValueItemProp)
            {
                throw std::runtime_error(RC::fmt("Ensure that all your map entries have both a 'Key' and 'Value' field within %S", 
                    Property->GetName().c_str()));
            }

            UECustom::FManagedValue ScopedPair;
            ScriptMapHelper.InitializePair(ScopedPair);

            KeyItemProp->CopyValue(KeyProp, ScopedPair.GetData());
            ValueItemProp->CopyValue(ValueProp, static_cast<uint8*>(ScopedPair.GetData()) + MapLayout.ValueOffset);

            ScriptMapHelper.Add(ScopedPair);
        }
    }

    JsonProperty::Type JsonArrayProperty::StaticType()
    {
        return JsonProperty::Type::Array;
    }
}