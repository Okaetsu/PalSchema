#pragma once

#include "Mods/Mod.h"
#include "Mods/Metadata.h"
#include "Mods/ModFile.h"
#include "Loader/ESchemaLoaderType.h"

namespace PS
{
    class FMod
    {
    public:
        FMod(const std::filesystem::path& InFolderPath);
        FMod(FMod&& Other) noexcept :
            FolderPath(Other.FolderPath),
            Metadata(Other.Metadata),
            FilesByLoaderType(std::move(Other.FilesByLoaderType)),
            TotalFileCount(Other.TotalFileCount)
        {
        };

        FModMetadata GetMetadata() const;

        bool IsEnabled() const;

        std::string GetId() const;

        std::string GetName() const;

        std::string GetDescription() const;

        std::string GetVersion() const;

        std::vector<std::string> GetDependencies() const;

        void GetFormattedMetadata(RC::Unreal::FString& OutString) const;

        RC::Unreal::uint32 GetTotalFileCount() const;

        void CollectModFilesByLoaderType(const ESchemaLoaderType::Type& LoaderType, std::vector<FModFile*>& ModFiles);

        bool Load(const std::vector<std::string>& RegisteredLoaderFolderNames);
    private:
        std::filesystem::path FolderPath{};
        FModMetadata Metadata{};
        std::unordered_map<ESchemaLoaderType::Type, std::vector<FModFile>> FilesByLoaderType{};
        RC::Unreal::uint32 TotalFileCount = 0;

        void LoadMetadata();
        bool LoadLoaderFiles(const std::vector<std::string>& RegisteredLoaderFolderNames);
        void LoadLoaderFile(const ESchemaLoaderType::Type& LoaderType, const std::filesystem::path& File);
    };
}