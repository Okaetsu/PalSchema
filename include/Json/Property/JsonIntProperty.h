#pragma once

#include "Json/Property/JsonProperty.h"

namespace PS
{
    class JsonIntProperty : public JsonProperty
    {
    public:
        JsonIntProperty();
        JsonIntProperty(const RC::Unreal::FString& InName);
        virtual ~JsonIntProperty() {};

        const RC::Unreal::int64& GetValue() const;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        virtual bool Parse(const nlohmann::json& Data) override final;
    private:
        RC::Unreal::int64 InnerValue{};
    };
}