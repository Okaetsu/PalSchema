#include "Loader/PalResourceLoader.h"
#include "Unreal/FString.hpp"
#include "Unreal/NameTypes.hpp"
#include "SDK/Classes/KismetRenderingLibrary.h"
#include "SDK/Classes/KismetSystemLibrary.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include <set>

using namespace RC;
using namespace RC::Unreal;
namespace fs = std::filesystem;

namespace Palworld {
    PalResourceLoader::PalResourceLoader()
    {
    }
    PalResourceLoader::~PalResourceLoader()
    {
    }

    void PalResourceLoader::Load(const std::filesystem::path& modPath)
    {
        if (!fs::is_directory(modPath))
        {
            return;
        }

        auto modName = modPath.stem().native();
        UnregisterResourceAssets(modName);

        auto resourcesPath = modPath / "resources";
        if (!fs::is_directory(resourcesPath))
        {
            return;
        }

        LoadImages(modName, resourcesPath);
    }

    void PalResourceLoader::RegisterResourceAsset(const std::filesystem::path::string_type& modName, RC::Unreal::UObject* resource)
    {
        auto modResourcesIt = m_loadedResourcesMap.find(modName);
        if (modResourcesIt != m_loadedResourcesMap.end())
        {
            auto& modResources = modResourcesIt->second;
            modResources.push_back(resource);
        }
        else
        {
            std::vector<UObject*> newResourcesContainer;
            newResourcesContainer.push_back(resource);
            m_loadedResourcesMap.emplace(modName, newResourcesContainer);
        }
    }

    void PalResourceLoader::UnregisterResourceAssets(const std::filesystem::path::string_type& modName)
    {
        auto modResourcesIt = m_loadedResourcesMap.find(modName);
        if (modResourcesIt != m_loadedResourcesMap.end())
        {
            auto& modResources = modResourcesIt->second;
            for (auto& loadedResource : modResources)
            {
                // We have to rename here, because CollectGarbage happens at the end of a frame and from testing, it happens after we add a new resource -
                // which isn't quick enough. You're not allowed to rename an asset to something that already exists, otherwise UE will crash.
                auto tempName = std::format(TEXT("{}-Temp"), loadedResource->GetFullName());
                loadedResource->Rename(tempName.c_str());
                loadedResource->ClearRootSet();
            }
            modResources.clear();

            // Manually calling GC here. It's fine because UnregisterResourceAssets only happens when auto-reloading or reloading PalSchema mods.
            UECustom::UKismetSystemLibrary::CollectGarbage();
        }
    }

    void PalResourceLoader::UnregisterResourceAssets()
    {
        for (auto& [modName, modResources] : m_loadedResourcesMap)
        {
            for (auto& loadedResource : modResources)
            {
                // We have to rename here, because CollectGarbage happens at the end of a frame and from testing, it happens after we add a new resource -
                // which isn't quick enough. You're not allowed to rename an asset to something that already exists, otherwise UE will crash.
                auto tempName = std::format(TEXT("{}-Temp"), loadedResource->GetFullName());
                loadedResource->Rename(tempName.c_str());
                loadedResource->ClearRootSet();
            }
            modResources.clear();
        }

        // Manually calling GC here. It's fine because UnregisterResourceAssets only happens when auto-reloading or reloading PalSchema mods.
        UECustom::UKismetSystemLibrary::CollectGarbage();
    }

    void PalResourceLoader::LoadImages(const std::filesystem::path::string_type& modName, const std::filesystem::path& resourcesPath)
    {
        // More formats are supported by UE, but we should stick to the commonly used ones.
        const std::set<std::string> supportedExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };

        auto imagesPath = resourcesPath / "images";
        if (!fs::is_directory(imagesPath))
        {
            return;
        }

        for (const auto& file : fs::directory_iterator(imagesPath)) {
            try
            {
                auto filePath = file.path();
                if (filePath.has_extension())
                {
                    auto extensionName = filePath.extension().string();
                    if (supportedExtensions.count(extensionName))
                    {
                        LoadImage(modName, filePath);
                    }
                }
            }
            catch (const std::exception&)
            {
                throw;
            }
        }
    }

    void PalResourceLoader::LoadImage(const std::filesystem::path::string_type& modName, const std::filesystem::path& imagePath)
    {
        auto packagePath = std::format(TEXT("PalSchema/Resources/{}/{}"), modName, imagePath.stem().native());
        auto newTexture = UECustom::UKismetRenderingLibrary::ImportFileAsTexture2D(nullptr, FString(imagePath.c_str()));
        newTexture->SetRootSet();

        newTexture->Rename(packagePath.c_str()); // becomes "/Engine/Transient.PalSchema/Resources/modname/resourcename"

        RegisterResourceAsset(modName, newTexture);

        PS::Log<LogLevel::Normal>(STR("Registered Image Resource '{}'"), packagePath);
    }
}
