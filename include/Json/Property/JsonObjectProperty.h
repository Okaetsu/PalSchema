#pragma once

#include "Json/Property/JsonProperty.h"
#include "Unreal/Core/Containers/Map.hpp"

namespace RC::Unreal
{
    class FStructProperty;
    class FObjectProperty;
}

namespace PS
{
    class JsonObjectProperty : public JsonProperty
    {
    public:
        JsonObjectProperty();
        JsonObjectProperty(const RC::Unreal::FString& InName);
        virtual ~JsonObjectProperty() {};

        virtual bool Parse(const nlohmann::ordered_json& Data) override final;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;

        void AddProperty(std::unique_ptr<JsonProperty> NewProperty);

        JsonProperty* GetPropertyByName(const RC::Unreal::FName& Name);

        void ForEachProperty(const std::function<void(const RC::Unreal::FName&, JsonProperty*)>& Callback);
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        RC::Unreal::TMap<RC::Unreal::FName, std::unique_ptr<JsonProperty>> Map;

        void CopyStructValue(RC::Unreal::FStructProperty* Property, void* Container);
        void CopyObjectValue(RC::Unreal::FObjectProperty* Property, void* Container);
    };
}