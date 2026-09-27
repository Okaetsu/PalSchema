#pragma once

#include "Unreal/NameTypes.hpp"
#include "Unreal/Core/Containers/Map.hpp"
#include "Json/Property/JsonProperty.h"
#include "nlohmann/json.hpp"
#include "Mods/Metadata.h"

namespace PS
{
    class JsonArrayProperty;
    class JsonObjectProperty;

    enum class EModFileType : RC::Unreal::uint8
    {
        Unknown,
        Array, // File is wrapped in []
        Object // File is wrapped in {}
    };

    class FModFile
    {
    public:
        FModFile(const FModMetadata& InMetadata, const nlohmann::json& Data);

        FModFile(FModFile&& Other) noexcept :
            Properties(std::move(Other.Properties)),
            FileType(Other.FileType),
            Metadata(Other.Metadata)
        {
        };

        FModMetadata GetMetadata() const;

        EModFileType GetFileType() const;

        JsonArrayProperty* GetAsArray();

        JsonObjectProperty* GetAsObject();
    private:
        const FModMetadata& Metadata;
        EModFileType FileType{};
        std::unique_ptr<JsonProperty> Properties;

        void Read(const nlohmann::json& Data);

        bool ParseArray(const nlohmann::json& Data);

        bool ParseObject(const nlohmann::json& Data);
    };
}