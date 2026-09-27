#pragma once

#include "Json/Property/JsonProperty.h"

namespace RC::Unreal
{
    class FArrayProperty;
    class FMapProperty;
}

namespace PS
{
    enum class EArrayOperationMode : RC::Unreal::uint8
    {
        Replace,
        Append
    };

    class JsonArrayProperty : public JsonProperty
    {
    public:
        JsonArrayProperty();
        JsonArrayProperty(const RC::Unreal::FString& InName);
        virtual ~JsonArrayProperty() {};

        virtual bool Parse(const nlohmann::ordered_json& Data) override final;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;

        EArrayOperationMode GetArrayOperationMode() const;

        void AddProperty(std::unique_ptr<JsonProperty> NewProperty);

        void ForEachProperty(const std::function<void(JsonProperty*)>& Callback);
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        void ParseAsArray(const nlohmann::ordered_json& Data);
        void ParseAsObject(const nlohmann::ordered_json& Data);
    private:
        EArrayOperationMode ArrayOperationMode = EArrayOperationMode::Replace;
        RC::Unreal::TArray<std::unique_ptr<JsonProperty>> Items;

        void CopyArrayValue(RC::Unreal::FArrayProperty* Property, void* Container);
        void CopyMapValue(RC::Unreal::FMapProperty* Property, void* Container);
    };
}