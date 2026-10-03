#pragma once

#include "nlohmann/json.hpp"
#include "Unreal/NameTypes.hpp"
#include "Unreal/Core/Containers/FString.hpp"

namespace RC::Unreal
{
    class FProperty;
}

namespace PS
{
    class JsonProperty
    {
    public:
        enum class Type : RC::Unreal::uint8
        {
            Undefined,
            String,
            Integer,
            Float,
            Bool,
            Null,
            Object,
            Array
        };
    public:
        JsonProperty(const JsonProperty::Type& Type);
        JsonProperty(const RC::Unreal::FString& InName, const JsonProperty::Type& Type);

        const RC::Unreal::FName& GetName() const;

        JsonProperty::Type GetType() const;

        RC::Unreal::FString GetTypeString() const;

        bool IsString() const;
        bool IsNumeric() const;
        bool IsInteger() const;
        bool IsFloat() const;
        bool IsBool() const;
        bool IsNull() const;
        bool IsObject() const;
        bool IsArray() const;

        void Dump(RC::Unreal::FString& OutString);
    protected:
        void PrintIndents(RC::Unreal::FString& OutString, int Indent);
    public:
        virtual bool Parse(const nlohmann::ordered_json& Data) = 0;

        virtual void CopyValue(RC::Unreal::FProperty* Property, void* Container) = 0;
    public:
        static std::unique_ptr<JsonProperty> CreateProperty(const nlohmann::json& Data);
        static std::unique_ptr<JsonProperty> CreateProperty(const std::string& InName, const nlohmann::json& Data);
        static std::unique_ptr<JsonProperty> CreateProperty(const RC::Unreal::FString& InName, const nlohmann::json& Data);
    protected:
        friend class JsonArrayProperty;
        friend class JsonObjectProperty;

        virtual void Print(RC::Unreal::FString& OutString, int Indent) = 0;
    private:
        RC::Unreal::FName Name = RC::Unreal::NAME_None;
        JsonProperty::Type PropertyType = JsonProperty::Type::Undefined;
    };
}