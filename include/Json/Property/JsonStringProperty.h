#pragma once

#include "Json/Property/JsonProperty.h"

namespace RC::Unreal
{
    class FObjectProperty;
    class FSoftObjectProperty;
    class FEnumProperty;
}

namespace PS
{
    class JsonStringProperty : public JsonProperty
    {
    public:
        JsonStringProperty();
        JsonStringProperty(const RC::Unreal::FString& InName);
        virtual ~JsonStringProperty() {};

        const RC::Unreal::FString& GetValue() const;

        virtual bool Parse(const nlohmann::ordered_json& Data) override final;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) override final;
    protected:
        virtual void Print(RC::Unreal::FString& OutString, int Indent) override final;
    private:
        RC::Unreal::FString InnerValue{};

        void ParseResourceString(RC::Unreal::FString& String);

        void CopyObjectValue(RC::Unreal::FObjectProperty* ObjectProperty, void* Container);
        void CopySoftObjectValue(RC::Unreal::FSoftObjectProperty* SoftObjectProperty, void* Container);
        void CopyEnumValue(RC::Unreal::FEnumProperty* EnumProperty, void* Container);
    private:
        inline static JsonProperty::Type StaticType();
    };
}