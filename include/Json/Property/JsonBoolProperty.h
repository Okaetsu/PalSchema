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
        
        virtual bool Parse(const nlohmann::ordered_json& Data) override final;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        bool InnerValue{};
    private:
        inline static JsonProperty::Type StaticType();
    };
}