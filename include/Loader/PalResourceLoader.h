#pragma once

#include <filesystem>
#include <unordered_map>

namespace RC::Unreal {
    class UObject;
}

namespace Palworld {
    class PalResourceLoader {
    public:
        PalResourceLoader();
        ~PalResourceLoader();
        void Load(const std::filesystem::path& modPath);
    private:
        void RegisterResourceAsset(const std::filesystem::path::string_type& modName, RC::Unreal::UObject* resource);
        void UnregisterResourceAssets(const std::filesystem::path::string_type& modName);
        void UnregisterResourceAssets();

        void LoadImages(const std::filesystem::path::string_type& modName, const std::filesystem::path& resourcesPath);
        void LoadImage(const std::filesystem::path::string_type& modName, const std::filesystem::path& imagePath);

        std::unordered_map<std::filesystem::path::string_type, std::vector<RC::Unreal::UObject*>> m_loadedResourcesMap;
    };
}