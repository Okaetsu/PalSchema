#include "Mods/ModFile.h"
#include "Utility/Logging.h"
#include "Utility/JsonHelpers.h"
#include "Json/Property/JsonArrayProperty.h"
#include "Json/Property/JsonObjectProperty.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    FModFile::FModFile(const FModMetadata& InMetadata, const std::filesystem::path& InFilePath)
        : Metadata(InMetadata), FilePath(InFilePath)
    {
        nlohmann::ordered_json OutData;
        if (JsonHelpers::ParseJsonFileInPath(InFilePath, OutData))
        {
            Read(OutData);
        }
    }

    const std::filesystem::path& FModFile::GetFilePath() const
    {
        return FilePath;
    }

    FModMetadata FModFile::GetMetadata() const
    {
        return Metadata;
    }

    EModFileType FModFile::GetFileType() const
    {
        return FileType;
    }

    JsonArrayProperty* FModFile::GetAsArray()
    {
        if (Properties && Properties->GetType() == JsonProperty::Type::Array)
        {
            return static_cast<JsonArrayProperty*>(Properties.get());
        }

        return nullptr;
    }

    JsonObjectProperty* FModFile::GetAsObject()
    {
        if (Properties && Properties->GetType() == JsonProperty::Type::Object)
        {
            return static_cast<JsonObjectProperty*>(Properties.get());
        }

        return nullptr;
    }

    void FModFile::Read(const nlohmann::json& Data)
    {
        if (Data.is_array())
        {
            ParseArray(Data);
        }
        else if (Data.is_object())
        {
            ParseObject(Data);
        }
        else
        {
            throw std::runtime_error("File was not formatted correctly. Expected an array or an object.");
        }
    }

    bool FModFile::ParseArray(const nlohmann::json& Data)
    {
        std::unique_ptr<JsonArrayProperty> ArrayProp = std::make_unique<JsonArrayProperty>();

        for (const nlohmann::json& Item : Data)
        {
            std::unique_ptr<JsonProperty> NewProperty = JsonProperty::CreateProperty(Data);
            ArrayProp->AddProperty(std::move(NewProperty));
        }

        FileType = EModFileType::Array;
        Properties = std::move(ArrayProp);

        return true;
    }

    bool FModFile::ParseObject(const nlohmann::json& Data)
    {
        std::unique_ptr<JsonObjectProperty> ObjectProp = std::make_unique<JsonObjectProperty>();

        for (const auto& [Key, Value] : Data.items())
        {
            std::unique_ptr<JsonProperty> NewProperty = JsonProperty::CreateProperty(Key, Value);
            ObjectProp->AddProperty(std::move(NewProperty));
        }

        FileType = EModFileType::Object;
        Properties = std::move(ObjectProp);

        return true;
    }
}