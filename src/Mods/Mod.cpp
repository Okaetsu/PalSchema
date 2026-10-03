#include "Mods/Mod.h"
#include "Mods/ModFile.h"
#include "Utility/JsonHelpers.h"
#include "Utility/Logging.h"
#include "Utility/StringHelpers.h"
#include "glaze/glaze.hpp"
#include "Json/Property/JsonObjectProperty.h"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace PS
{
    FMod::FMod(const fs::path& InFolderPath) : FolderPath(InFolderPath)
    {
    }

    FModMetadata FMod::GetMetadata() const
    {
        return Metadata;
    }

    bool FMod::IsEnabled() const
    {
        return GetMetadata().enabled;
    }

    std::string FMod::GetId() const
    {
        return GetMetadata().mod_id;
    }

    std::string FMod::GetName() const
    {
        return GetMetadata().name;
    }

    std::string FMod::GetDescription() const
    {
        return GetMetadata().description;
    }

    std::string FMod::GetVersion() const
    {
        return GetMetadata().version;
    }

    std::vector<std::string> FMod::GetDependencies() const
    {
        return GetMetadata().dependencies;
    }

    void FMod::GetFormattedMetadata(RC::Unreal::FString& OutString) const
    {
        GetMetadata().GetFormattedMetadata(OutString);
    }

    RC::Unreal::uint32 FMod::GetTotalFileCount() const
    {
        return TotalFileCount;
    }

    void FMod::CollectModFilesByLoaderType(const ESchemaLoaderType::Type& LoaderType, std::vector<FModFile*>& ModFiles)
    {
        auto It = FilesByLoaderType.find(LoaderType);
        if (It == FilesByLoaderType.end())
        {
            return;
        }

        for (auto& ModFile : It->second)
        {
            ModFiles.push_back(&ModFile);
        }
    }

    bool FMod::Load(const std::vector<std::string>& RegisteredLoaderFolderNames)
    {
        LoadMetadata();

        bool bSuccess = LoadFiles(RegisteredLoaderFolderNames);
        return bSuccess;
    }

    void FMod::LoadMetadata()
    {
        const fs::path MetadataFile = FolderPath / "metadata.json";
        std::string ModName = FolderPath.stem().string();

        if (fs::exists(MetadataFile))
        {
            auto ErrorCode = glz::read_file_json(Metadata, MetadataFile.string(), std::string{});
            if (ErrorCode)
            {
                std::string ErrorText = glz::format_error(ErrorCode);
                PS::Log<LogLevel::Error>(STR("Failed parsing metadata.json for {}. {}\n"), RC::to_generic_string(ModName), RC::to_generic_string(ErrorText));
            }
        }

        if (Metadata.name == "")
        {
            Metadata.name = ModName;
        }

        if (Metadata.mod_id == "")
        {
            Metadata.mod_id = Metadata.name;
        }

        StringHelpers::ToLowerCase(Metadata.mod_id);
    }

    bool FMod::LoadFiles(const std::vector<std::string>& RegisteredLoaderFolderNames)
    {
        bool bSuccess = true;

        for (auto& RegisteredLoaderFolderName : RegisteredLoaderFolderNames)
        {
            const fs::path LoaderPath = FolderPath / RegisteredLoaderFolderName;
            if (!fs::exists(LoaderPath))
            {
                continue;
            }

            if (RegisteredLoaderFolderName == "translations")
            {
                LoadGlobalLocalizationFiles();
                continue;
            }

            ESchemaLoaderType::Type LoaderType = ESchemaLoaderType::GetTypeFromString(RegisteredLoaderFolderName);
            JsonHelpers::IterateJsonFilesInPath(LoaderPath, [&](const fs::path& File)
            {
                try
                {
                    LoadFile(LoaderType, File);
                }
                catch (const std::exception& e)
                {
                    bSuccess = false;
                    PS::Log<LogLevel::Error>(STR("{}: Failed to load {} - {}\n"), 
                        RC::to_generic_string(GetName()), RC::to_generic_string(File), RC::to_generic_string(e.what()));
                }
            });
        }

        return bSuccess;
    }

    void FMod::LoadFile(const ESchemaLoaderType::Type& LoaderType, const std::filesystem::path& File)
    {
        auto ModFile = FModFile(Metadata, File);
        auto [It, WasInserted] = FilesByLoaderType.try_emplace(LoaderType);
        It->second.push_back(std::move(ModFile));
        TotalFileCount++;
    }

    void FMod::LoadGlobalLocalizationFiles()
    {
        LoadLocalizationFiles(FString(TEXT("global")));
    }

    void FMod::LoadLocalizationFiles(const RC::Unreal::FString& LanguageCode)
    {
        const fs::path LocalizationPath = FolderPath / "translations" / *LanguageCode;
        int32 LocalizationFileCount = 0;

        JsonHelpers::IterateJsonFilesInPath(LocalizationPath, [&](const fs::path& File)
        {
            try
            {
                LoadLocalizationFile(File);
                LocalizationFileCount++;
            }
            catch (const std::exception& e)
            {
                PS::Log<LogLevel::Error>(STR("[{}] Failed to load {} - {}\n"),
                    RC::to_generic_string(GetId()), RC::to_generic_string(File), RC::to_generic_string(e.what()));
            }
        });

        if (LocalizationFileCount > 0)
        {
            RC::StringType Plural = LocalizationFileCount > 1 ? TEXT("s") : TEXT("");
            PS::Log<LogLevel::Normal>(STR("[{}] Registered {} localization file{} for language '{}'.\n"),
                RC::to_generic_string(GetId()), LocalizationFileCount, Plural, *LanguageCode);
        }
    }

    void FMod::LoadLocalizationFile(const std::filesystem::path& File)
    {
        auto ModFile = FModFile(Metadata, File);
        auto [It, WasInserted] = FilesByLoaderType.try_emplace(ESchemaLoaderType::Type::Language);
        It->second.push_back(std::move(ModFile));
        TotalFileCount++;
    }
}