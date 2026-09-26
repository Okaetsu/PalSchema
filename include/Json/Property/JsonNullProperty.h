#pragma once

#include "Json/Property/JsonProperty.h"

namespace PS
{
    class JsonNullProperty : public JsonProperty
    {
    public:
        JsonNullProperty();
        JsonNullProperty(const RC::Unreal::FString& InName);
        virtual ~JsonNullProperty() {};

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        virtual bool Parse(const nlohmann::json& Data) override final;
    };
}