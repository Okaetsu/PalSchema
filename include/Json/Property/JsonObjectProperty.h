#pragma once

#include "Json/Property/JsonProperty.h"
#include "Unreal/Core/Containers/Map.hpp"

namespace PS
{
    class JsonObjectProperty : public JsonProperty
    {
    public:
        JsonObjectProperty();
        JsonObjectProperty(const RC::Unreal::FString& InName);
        virtual ~JsonObjectProperty() {};

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;

        void AddProperty(std::unique_ptr<JsonProperty> NewProperty);

        void ForEachProperty(const std::function<void(const RC::Unreal::FName&, JsonProperty*)>& Callback);
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        virtual bool Parse(const nlohmann::json& Data) override final;
    private:
        RC::Unreal::TMap<RC::Unreal::FName, std::unique_ptr<JsonProperty>> Map;
    };
}