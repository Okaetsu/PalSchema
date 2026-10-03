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

        virtual bool Parse(const nlohmann::ordered_json& Data) override final;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        double InnerValue{};
    private:
        inline static JsonProperty::Type StaticType();
    };
}