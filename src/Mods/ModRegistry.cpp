#include "Mods/ModRegistry.h"
#include "Utility/Logging.h"
#include "Utility/StringHelpers.h"
#include "glaze/glaze.hpp"
#include "UE4SSProgram.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace PS
{
    void FModRegistry::PreloadMods(const fs::path& InModsPath)
    {
        if (!fs::exists(InModsPath))
        {
            return;
        }

        ModsPath = InModsPath;

        std::unordered_set<std::string> ReservedModIds;
        std::vector<std::unique_ptr<FMod>> SortedMods;
        IterateModsFolder([&](const fs::path& ModPath, const RC::StringType& ModFolderName)
        {
            std::unique_ptr<FMod> NewMod = PreloadMod(ModPath, ModFolderName);
            if (!NewMod)
            {
                return;
            }

            std::string ModId = NewMod->GetId();
            if (ReservedModIds.contains(ModId))
            {
                PS::Log<LogLevel::Error>(STR("Mod id {} was already occupied, unable to load {}.\n"), RC::to_generic_string(ModId), ModFolderName);
                return;
            }

            ReservedModIds.insert(ModId);
            SortedMods.push_back(std::move(NewMod));
        });

        // A-Z sorting by mod_id
        std::sort(SortedMods.begin(), SortedMods.end(), [](std::unique_ptr<FMod>& A, std::unique_ptr<FMod>& B) {
            return A->GetId() < B->GetId();
        });

        SortMods(SortedMods);

        for (std::unique_ptr<FMod>& SortedMod : SortedMods)
        {
            FString OutFormattedMetadata;
            SortedMod->GetFormattedMetadata(OutFormattedMetadata);
            PS::Log<RC::LogLevel::Normal>(STR("Loaded mod: {}\n"), *OutFormattedMetadata);
        }

        ModList = std::move(SortedMods);
    }

    void FModRegistry::RegisterLoaderFolder(const std::string& LoaderFolderName)
    {
        RegisteredLoaderFolderNames.push_back(LoaderFolderName);
    }

    bool FModRegistry::CollectModFilesByLoaderType(const ESchemaLoaderType::Type& LoaderType, std::vector<FModFile*>& ModFiles)
    {
        for (std::unique_ptr<FMod>& Mod : ModList)
        {
            Mod->CollectModFilesByLoaderType(LoaderType, ModFiles);
        }

        return ModFiles.size() > 0;
    }

    void FModRegistry::IterateModsFolder(const std::function<void(const fs::path&, const RC::StringType&)>& Callback)
    {
        for (const auto& Entry : fs::directory_iterator(ModsPath)) {
            if (Entry.is_directory())
            {
                auto& Path = Entry.path();
                auto FolderName = Path.stem().native();
                Callback(Path, FolderName);
            }
        }
    }

    std::unique_ptr<FMod> FModRegistry::PreloadMod(const std::filesystem::path& ModPath, const RC::StringType& ModFolderName)
    {
        auto NewMod = std::make_unique<FMod>(ModPath);
        bool bWasLoadSuccessful = NewMod->Load(RegisteredLoaderFolderNames);
        if (!bWasLoadSuccessful)
        {
            PS::Log<LogLevel::Error>(STR("{} failed to load, skipping.\n"), ModFolderName);
            return nullptr;
        }

        // We should have the actual name of the mod from the metadata.json file if it was successfully read.
        // Otherwise this will just be the name of the mod folder (Same as ModFolderName)
        std::string ModName = NewMod->GetName();

        if (NewMod->GetTotalFileCount() == 0)
        {
            PS::Log<LogLevel::Warning>(STR("{} has no files to load, skipping.\n"), RC::to_generic_string(ModName));
            return nullptr;
        }

        if (!NewMod->IsEnabled())
        {
            PS::Log<LogLevel::Normal>(STR("{} is disabled, skipping.\n"), RC::to_generic_string(ModName));
            return nullptr;
        }

        return std::move(NewMod);
    }

    void FModRegistry::SortMods(std::vector<std::unique_ptr<FMod>>& OutSortedMods)
    {
        fs::path WorkingDir = fs::path(UE4SSProgram::get_program().get_working_directory()) / "Mods" / "PalSchema";
        fs::path LoadOrderFile = WorkingDir / "load_order.json";

        if (!fs::exists(LoadOrderFile))
        {
            PS::Log<LogLevel::Normal>(STR("load_order.json is not present, loading mods in alphabetical order.\n"));
            return;
        }

        std::vector<std::string> LoadOrderList;
        glz::error_ctx ErrorCode = glz::read_file_json(LoadOrderList, LoadOrderFile.string(), std::string{});
        if (ErrorCode)
        {
            std::string ErrorText = glz::format_error(ErrorCode);
            PS::Log<LogLevel::Error>(STR("Failed parsing load_order.json, loading mods in alphabetical order. {}\n"), RC::to_generic_string(ErrorText));
            return;
        }

        StringHelpers::ToLowerCase(LoadOrderList);

        PS::Log<LogLevel::Normal>(STR("load_order.json found, sorting mods...\n"));

        // We'll want to move the mods temporarily so we can use OutSortedMods to make it easier to keep mods in alphabetical order later.
        std::vector<std::unique_ptr<FMod>> TempSortedMods = std::move(OutSortedMods);
        std::unordered_map<std::string, std::unique_ptr<FMod>> SortingMap;

        // Only move mods that are listed in load_order.json into SortingMap.
        // This makes it so that we don't have to sort the list at the end again.
        std::erase_if(TempSortedMods, [&](std::unique_ptr<FMod>& TempSortedMod)
        {
            std::string ModId = TempSortedMod->GetId();

            auto It = std::find(LoadOrderList.begin(), LoadOrderList.end(), ModId);
            bool bShouldDelete = It != LoadOrderList.end();
            if (bShouldDelete)
            {
                SortingMap.emplace(ModId, std::move(TempSortedMod));
            }

            return bShouldDelete;
        });

        PS::Log<LogLevel::Normal>(STR("Found {} mod(s) listed by load_order.json.\n"), SortingMap.size());

        // Sort the mods listed in load_order.json (Top to bottom order)
        for (std::string& LoadOrderEntry : LoadOrderList)
        {
            auto It = SortingMap.find(LoadOrderEntry);
            if (It != SortingMap.end())
            {
                std::unique_ptr<FMod>& Mod = It->second;
                OutSortedMods.push_back(std::move(Mod));
                SortingMap.erase(LoadOrderEntry);
            }
        }

        // Finally move the remaining original A-Z sorted mods back into OutSortedMods right after the mods that were sorted by load_order.json.
        for (std::unique_ptr<FMod>& TempSortedMod : TempSortedMods)
        {
            OutSortedMods.push_back(std::move(TempSortedMod));
        }
    }
}