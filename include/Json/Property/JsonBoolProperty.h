#pragma once

#include "Json/Property/JsonProperty.h"

namespace PS
{
    class JsonBoolProperty : public JsonProperty
    {
    public:
        JsonBoolProperty();
        JsonBoolProperty(const RC::Unreal::FString& InName);
        virtual ~JsonBoolProperty() {};

        const bool& GetValue() const;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        virtual bool Parse(const nlohmann::json& Data) override final;
    private:
        bool InnerValue{};
    };
}