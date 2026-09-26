#pragma once

#include "Json/Property/JsonProperty.h"

namespace PS
{
    class JsonFloatProperty : public JsonProperty
    {
    public:
        JsonFloatProperty();
        JsonFloatProperty(const RC::Unreal::FString& InName);
        virtual ~JsonFloatProperty() {};

        const double& GetValue() const;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        virtual bool Parse(const nlohmann::json& Data) override final;
    private:
        double InnerValue{};
    };
}