#pragma once

#include "Mods/Mod.h"
#include "Loader/ESchemaLoaderType.h"

namespace PS
{
    class FModRegistry
    {
    public:
        void PreloadMods(const std::filesystem::path& InModsPath);

        void RegisterLoaderFolder(const std::string& LoaderFolderName);

        bool CollectModFilesByLoaderType(const ESchemaLoaderType::Type& LoaderType, std::vector<FModFile*>& ModFiles);
    private:
        std::filesystem::path ModsPath;
        std::vector<std::unique_ptr<FMod>> ModList;
        std::vector<std::string> RegisteredLoaderFolderNames;

        void IterateModsFolder(const std::function<void(const std::filesystem::path&, const RC::StringType&)>& Callback);

        std::unique_ptr<FMod> PreloadMod(const std::filesystem::path& ModPath, const RC::StringType& ModFolderName);

        void SortMods(std::vector<std::unique_ptr<FMod>>& OutSortedMods);
    };
}