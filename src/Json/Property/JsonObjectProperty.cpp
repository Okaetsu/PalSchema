#include "Json/Property/JsonObjectProperty.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "SDK/Helper/PropertyHelper.h"

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

    bool JsonObjectProperty::Parse(const nlohmann::ordered_json& Data)
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

    void JsonObjectProperty::CopyValue(FProperty* Property, void* Container)
    {
        using namespace Palworld::PropertyHelper;

        if (FStructProperty* StructProperty = CastProperty<FStructProperty>(Property))
        {
            CopyStructValue(StructProperty, Container);
        }
        else if (FObjectProperty* ObjectProperty = CastProperty<FObjectProperty>(Property))
        {
            CopyObjectValue(ObjectProperty, Container);
        }
        else
        {
            throw std::runtime_error(RC::fmt("Unsupported Type '%S' for Property '%S'",
                *GetTypeString(), Property->GetName().c_str()));
        }
    }

    void JsonObjectProperty::AddProperty(std::unique_ptr<JsonProperty> NewProperty)
    {
        Map.Add(NewProperty->GetName(), std::move(NewProperty));
    }

    JsonProperty* JsonObjectProperty::GetPropertyByName(const RC::Unreal::FName& Name)
    {
        auto It = Map.Find(Name);
        if (It)
        {
            return It->get();
        }

        return nullptr;
    }

    void JsonObjectProperty::ForEachProperty(const std::function<void(const FName&, JsonProperty*)>& Callback)
    {
        for (const auto& Pair : Map)
        {
            Callback(Pair.Key, Pair.Value.get());
        }
    }

    void JsonObjectProperty::Print(FString& OutString, int Indent)
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

    void JsonObjectProperty::CopyStructValue(FStructProperty* Property, void* Container)
    {
        UScriptStruct* ScriptStruct = Property->GetStruct();
        if (!ScriptStruct)
        {
            throw std::runtime_error(RC::fmt("Failed to get ScriptStruct from %S", Property->GetName().c_str()));
        }

        for (auto& Pair : Map)
        {
            FProperty* InnerProperty = ScriptStruct->GetPropertyByNameInChain(Pair.Key);
            if (!InnerProperty)
            {
                PS::Log<LogLevel::Warning>(STR("Property {} was not found in {}.\n"), Pair.Key.ToString(), ScriptStruct->GetName());
                continue;
            }

            void* InnerValuePtr = InnerProperty->ContainerPtrToValuePtr<void>(Container);
            Pair.Value->CopyValue(InnerProperty, InnerValuePtr);
        }
    }

    void JsonObjectProperty::CopyObjectValue(RC::Unreal::FObjectProperty* Property, void* Container)
    {
        UObject* Object = Property->GetObjectPropertyValue(Container);
        if (!Object)
        {
            throw std::runtime_error(RC::fmt("Object was null in '%S'", Property->GetName().c_str()));
        }

        for (auto& Pair : Map)
        {
            FProperty* InnerProperty = Object->GetPropertyByNameInChain(Pair.Key);
            if (!InnerProperty)
            {
                PS::Log<LogLevel::Warning>(STR("Property {} was not found in {}.\n"), Pair.Key.ToString(), Object->GetName());
                continue;
            }

            void* InnerValuePtr = InnerProperty->ContainerPtrToValuePtr<void>(Object);
            Pair.Value->CopyValue(InnerProperty, InnerValuePtr);
        }
    }
}