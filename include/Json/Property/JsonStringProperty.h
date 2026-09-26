#pragma once

#include "Json/Property/JsonProperty.h"

namespace PS
{
    class JsonStringProperty : public JsonProperty
    {
    public:
        JsonStringProperty();
        JsonStringProperty(const RC::Unreal::FString& InName);
        virtual ~JsonStringProperty() {};

        const RC::Unreal::FString& GetValue() const;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;        
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        virtual bool Parse(const nlohmann::json& Data) override final;
    private:
        RC::Unreal::FString InnerValue{};
    };
}