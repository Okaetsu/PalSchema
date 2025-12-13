#include <fstream>
#include <filesystem>
#include "Unreal/UClass.hpp"
#include "Unreal/UFunction.hpp"
#include "Unreal/Hooks.hpp"
#include "Utility/Config.h"
#include "Utility/Logging.h"
#include "SDK/Classes/Async.h"
#include "SDK/Classes/Custom/UDataTableStore.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "SDK/Classes/UCompositeDataTable.h"
#include "SDK/Classes/UWorldPartitionRuntimeLevelStreamingCell.h"
#include "SDK/Classes/PalUtility.h"
#include "SDK/PalSignatures.h"
#include "SDK/StaticClassStorage.h"
#include "SDK/UnrealOffsets.h"
#include "UE4SSProgram.hpp"
#include "Loader/PalMainLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace constants {
    // FOLDERS //
    constexpr std::string appearanceFolder      = "appearance";
    constexpr std::string blueprintsFolder      = "blueprints";
    constexpr std::string buildingsFolder       = "buildings";
    constexpr std::string enumsFolder           = "enums";
    constexpr std::string helpguideFolder       = "helpguide";
    constexpr std::string itemsFolder           = "items";
    constexpr std::string npcsFolder            = "npcs";
    constexpr std::string palsFolder            = "pals";
    constexpr std::string rawFolder             = "raw";
    constexpr std::string skinsFolder           = "skins";
    constexpr std::string translationsFolder    = "translations";
    constexpr std::string resourcesFolder       = "resources";
    constexpr std::string spawnsFolder          = "spawns";
}

namespace Palworld {
    PalMainLoader::PalMainLoader() {}

    PalMainLoader::~PalMainLoader()
    {
        auto expected1 = DatatableSerialize_Hook.disable();
        DatatableSerialize_Hook = {};

        auto expected2 = GameInstanceInit_Hook.disable();
        GameInstanceInit_Hook = {};

        auto expected3 = PostLoad_Hook.disable();
        PostLoad_Hook = {};

        auto expected4 = GetPakFolders_Hook.disable();
        GetPakFolders_Hook = {};

        auto expected5 = StaticItemDataTable_Get_Hook.disable();
        StaticItemDataTable_Get_Hook = {};

        auto expected6 = WorldCleanUp_Hook.disable();
        WorldCleanUp_Hook = {};

        DatatableSerializeCallbacks.clear();
        GameInstanceInitCallbacks.clear();
        PostLoadCallbacks.clear();
        GetPakFoldersCallback.clear();
        WorldCleanUp_Callbacks.clear();
    }

    void PalMainLoader::PreInitialize()
    {
        HookDatatableSerialize();
        HookStaticItemDataTable_Get();
        HookWorldCleanup();
        SetupAlternativePakPathReader();
    }

    void PalMainLoader::Initialize()
	{
        SetupAutoReload();
	}

    void PalMainLoader::ReloadMods()
    {
        PS::Log<LogLevel::Normal>(STR("Reloading mods...\n"));

        auto loadErrorCallback = [](const fs::path::string_type& modName, const std::exception& e) {
            PS::Log<LogLevel::Error>(STR("Failed to reload mod {} - {}\n"), modName, RC::to_generic_string(e.what()));
        };
        Load(loadErrorCallback);

        PS::Log<LogLevel::Normal>(STR("Finished reloading mods.\n"));
    }

    void PalMainLoader::HookDatatableSerialize()
    {
        auto DatatableSerializeFuncPtr = Palworld::SignatureManager::GetSignature("UDataTable::Serialize");
        if (!DatatableSerializeFuncPtr)
        {
            PS::Log<LogLevel::Error>(STR("Unable to initialize PalSchema core, signature for UDataTable::Serialize is outdated.\n"));
            return;
        }

        DatatableSerialize_Hook = safetyhook::create_inline(reinterpret_cast<void*>(DatatableSerializeFuncPtr),
            OnDataTableSerialized);

        DatatableSerializeCallbacks.push_back([&](RC::Unreal::UDataTable* Table) {
            InitCore();
            UECustom::UDataTableStore::Store(Table);
            RawTableLoader.OnDataTableChanged(Table);
        });

        PS::Log<LogLevel::Verbose>(STR("Core pre-initialized.\n"));
    }

    void PalMainLoader::HookStaticItemDataTable_Get()
    {
        auto StaticItemDataTable_GetFuncPtr = Palworld::SignatureManager::GetSignature("UPalStaticItemDataTable::Get");
        if (!StaticItemDataTable_GetFuncPtr)
        {
            PS::Log<LogLevel::Error>(STR("Unable to apply dummy item fix, signature for UPalStaticItemDataTable::Get is outdated.\n"));
            return;
        }

        StaticItemDataTable_Get_Hook = safetyhook::create_inline(reinterpret_cast<void*>(StaticItemDataTable_GetFuncPtr),
            StaticItemDataTable_Get);

        PS::Log<LogLevel::Verbose>(STR("Dummy item fix applied.\n"));
    }

    void PalMainLoader::HookWorldCleanup()
    {
        auto World_CleanupWorld_FuncPtr = Palworld::SignatureManager::GetSignature("UWorld::CleanupWorld");
        if (!World_CleanupWorld_FuncPtr)
        {
            PS::Log<LogLevel::Error>(STR("Unable to hook UWorld::CleanupWorld, signature is outdated. Custom spawns will not work.\n"));
            return;
        }

        WorldCleanUp_Hook = safetyhook::create_inline(reinterpret_cast<void*>(World_CleanupWorld_FuncPtr),
            UWorld_CleanupWorld);

        WorldCleanUp_Callbacks.push_back([&](RC::Unreal::UWorld* WorldToCleanup) {
            SpawnLoader.OnWorldCleanup(WorldToCleanup);
        });
    }

    void PalMainLoader::SetupAutoReload()
    {
        auto config = PS::PSConfig::Get();
        if (!config->IsAutoReloadEnabled()) return;

        PS::Log<LogLevel::Normal>(STR("Auto-reload is enabled.\n"));

        m_fileWatch = std::make_unique<filewatch::FileWatch<std::wstring>>(
            fs::path(UE4SSProgram::get_program().get_working_directory()) / "Mods" / "PalSchema" / "mods",
            std::wregex(L".*\\.(json|jsonc)"),
            [&](const std::wstring& path, const filewatch::Event change_type) {
                if (change_type == filewatch::Event::modified)
                {
                    auto ue4ssPath = fs::path(UE4SSProgram::get_program().get_working_directory());
                    auto modsPath = ue4ssPath / "Mods" / "PalSchema" / "mods";
                    auto modFilePath = modsPath / path;

                    auto subPath = fs::path(path);
                    auto it = subPath.begin();
                    if (std::distance(subPath.begin(), subPath.end()) < 2) {
                        return;
                    }

                    auto modName = it->native();
                    auto modPath = ue4ssPath / "Mods" / "PalSchema" / "mods" / modName;

                    std::advance(it, 1);
                    auto folderType = it->string();

                    std::ifstream f(modFilePath);
                    if (f.peek() == std::ifstream::traits_type::eof()) {
                        return;
                    }
                    f.close();

                    UECustom::AsyncTask(UECustom::ENamedThreads::GameThread, [this, path, folderType, modName, modPath, modFilePath]() {
                        try
                        {
                            ResourceLoader.Load(modPath);

                            ParseJsonFileInPath(modFilePath, [&](const nlohmann::json& data) {
                                if (folderType == constants::palsFolder)
                                {
                                    MonsterModLoader.Load(data);
                                }
                                else if (folderType == constants::appearanceFolder)
                                {
                                    AppearanceModLoader.Load(data);
                                }
                                else if (folderType == constants::buildingsFolder)
                                {
                                    BuildingModLoader.Load(data);
                                }
                                else if (folderType == constants::itemsFolder)
                                {
                                    ItemModLoader.Load(data);
                                }
                                else if (folderType == constants::skinsFolder)
                                {
                                    SkinModLoader.Load(data);
                                }
                                else if (folderType == constants::helpguideFolder)
                                {
                                    HelpGuideModLoader.Load(data);
                                }
                                else if (folderType == constants::translationsFolder)
                                {
                                    LanguageModLoader.Load(data);
                                }
                                else if (folderType == constants::blueprintsFolder)
                                {
                                    BlueprintModLoader.Load(data);
                                }
                                else if (folderType == constants::npcsFolder)
                                {
                                    HumanModLoader.Load(data);
                                }
                                else if (folderType == constants::rawFolder)
                                {
                                    RawTableLoader.Reload(data);
                                }
                                else if (folderType == constants::spawnsFolder)
                                {
                                    SpawnLoader.Reload(modName, data);
                                }
                            });

                            PS::Log<LogLevel::Normal>(STR("Auto-reloaded mod {}\n"), modName);
                        }
                        catch (const std::exception& e)
                        {
                            PS::Log<LogLevel::Error>(STR("Failed to auto-reload mod {} - {}\n"), modName, RC::to_generic_string(e.what()));
                        }
                    });
                }
            }
        );
    }

    void PalMainLoader::SetupAlternativePakPathReader()
    {
        auto GetPakFolders_Address = Palworld::SignatureManager::GetSignature("FPakPlatformFile::GetPakFolders");
        if (GetPakFolders_Address)
        {
            GetPakFolders_Hook = safetyhook::create_inline(reinterpret_cast<void*>(GetPakFolders_Address),
                GetPakFolders);
        }
        else
        {
            PS::Log<LogLevel::Error>(STR("Unable to setup additional .pak read directory, signature for FPakPlatformFile::GetPakFolders is outdated.\n"));
        }
    }

    void PalMainLoader::InitCore()
    {
        if (m_hasInit) return;
        m_hasInit = true;

        PS::Log<LogLevel::Verbose>(STR("Initializing Static Class Storage...\n"));
        Palworld::StaticClassStorage::Initialize();

        LoadCustomEnums();

        PS::Log<LogLevel::Verbose>(STR("Loading raw tables and blueprint mods...\n"));
        IterateModsFolder([&](const fs::path& modPath, const fs::path::string_type& modName)
        {
            try
            {
                PS::Log<RC::LogLevel::Normal>(STR("Loading mod: {}\n"), modName);

                auto rawFolder = modPath / constants::rawFolder;
                ParseJsonFilesInPath(rawFolder, [&](const nlohmann::json& data) {
                    RawTableLoader.Load(data);
                });

                auto blueprintFolder = modPath / constants::blueprintsFolder;
                ParseJsonFilesInPath(blueprintFolder, [&](const nlohmann::json& data) {
                    BlueprintModLoader.LoadSafe(data);
                });
            }
            catch (const std::exception& e)
            {
                PS::Log<LogLevel::Error>(STR("Failed to load mod {} - {}\n"), modName, RC::to_generic_string(e.what()));
            }
        });
        PS::Log<LogLevel::Verbose>(STR("Finished loading raw tables and blueprint mods.\n"));

        // Should in theory be more consistent than finding a signature for BlueprintGeneratedClass::PostLoad
        auto BlueprintGeneratedClass = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.BlueprintGeneratedClass"), false);
        if (!BlueprintGeneratedClass)
        {
            PS::Log<LogLevel::Error>(STR("Failed to find BlueprintGeneratedClass. Cannot hook PostLoad.\n"));
            return;
        }

        PS::Log<LogLevel::Verbose>(STR("Fetching default object for UBlueprintGeneratedClass...\n"));
        uintptr_t* BGCVTablePtr = *(uintptr_t**)BlueprintGeneratedClass->GetClassDefaultObject();
        void* PostLoadPtr = (void*)BGCVTablePtr[20];
        PS::Log<LogLevel::Verbose>(STR("Found UBlueprintGeneratedClass::PostLoad: {}\n"), PostLoadPtr);

        PostLoadCallbacks.push_back([&](UClass* BPGeneratedClass) {
            BlueprintModLoader.OnPostLoadDefaultObject(BPGeneratedClass, BPGeneratedClass->GetClassDefaultObject());
        });

        PostLoad_Hook = safetyhook::create_inline(PostLoadPtr,
            reinterpret_cast<void*>(PostLoad));

        auto PalGameInstanceClass = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Pal.PalGameInstance"));
        if (!PalGameInstanceClass)
        {
            PS::Log<LogLevel::Error>(STR("Failed to find PalGameInstance. Cannot hook OnGameInstanceInit.\n"));
            return;
        }

        PS::Log<LogLevel::Verbose>(STR("Fetching default object for UPalGameInstance...\n"));
        uintptr_t** PGIVTablePtr = *(uintptr_t***)PalGameInstanceClass->GetClassDefaultObject();
        void* GameInstanceInitPtr = (void*)PGIVTablePtr[90];
        PS::Log<LogLevel::Verbose>(STR("Found UPalGameInstance::Init: {}\n"), GameInstanceInitPtr);

        GameInstanceInitCallbacks.push_back([&](UObject* Instance) {
            InitLoaders();
        });

        GameInstanceInit_Hook = safetyhook::create_inline(GameInstanceInitPtr,
            reinterpret_cast<void*>(OnGameInstanceInit));

        auto onLevelShownFunction = UECustom::UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Engine.WorldPartitionRuntimeLevelStreamingCell:OnLevelShown"));
        onLevelShownFunction->RegisterPostHook([&](UnrealScriptFunctionCallableContext& Context, void* CustomData) {
            auto Cell = static_cast<UECustom::UWorldPartitionRuntimeLevelStreamingCell*>(Context.Context);
            SpawnLoader.OnCellLoaded(Cell);
        });

        auto onLevelHiddenFunction = UECustom::UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Engine.WorldPartitionRuntimeLevelStreamingCell:OnLevelHidden"));
        onLevelHiddenFunction->RegisterPostHook([&](UnrealScriptFunctionCallableContext& Context, void* CustomData) {
            auto Cell = static_cast<UECustom::UWorldPartitionRuntimeLevelStreamingCell*>(Context.Context);
            SpawnLoader.OnCellUnloaded(Cell);
        });

        PS::Log<LogLevel::Verbose>(STR("Initialized Core\n"));
    }

    void PalMainLoader::InitLoaders()
    {
        PS::Log<LogLevel::Verbose>(STR("Initializing UDataTable storage...\n"));
        UECustom::UDataTableStore::Initialize();

        PS::Log<LogLevel::Verbose>(STR("Initializing Loaders...\n"));
        LanguageModLoader.Initialize();
        MonsterModLoader.Initialize();
        HumanModLoader.Initialize();
        AppearanceModLoader.Initialize();
        BuildingModLoader.Initialize();
        ItemModLoader.Initialize();
        SkinModLoader.Initialize();
        HelpGuideModLoader.Initialize();
        SpawnLoader.Initialize();
        PS::Log<LogLevel::Verbose>(STR("Initialized Loaders\n"));
        PS::Log<LogLevel::Normal>(STR("Loading mods...\n"));

        auto loadErrorCallback = [](const fs::path::string_type& modName, const std::exception& e) {
            PS::Log<LogLevel::Error>(STR("Failed to load mod {} - {}\n"), modName, RC::to_generic_string(e.what()));
        };
        Load(loadErrorCallback);

        PS::Log<LogLevel::Normal>(STR("Finished loading mods.\n"));
    }

    void PalMainLoader::Load(const std::function<void(const fs::path::string_type&, const std::exception&)>& errorCallback)
	{
        IterateModsFolder([&](const fs::path& modPath, const fs::path::string_type& modName)
        {
            try
            {
                PS::Log<RC::LogLevel::Normal>(STR("Loading mod: {}\n"), modName);

                ResourceLoader.Load(modPath);

                auto translationsFolder = modPath / constants::translationsFolder;
                LoadLanguageMods(translationsFolder);
                
                auto palFolder = modPath / constants::palsFolder;
                ParseJsonFilesInPath(palFolder, [&](const nlohmann::json& data) {
                    MonsterModLoader.Load(data);
                });

                auto npcFolder = modPath / constants::npcsFolder;
                ParseJsonFilesInPath(npcFolder, [&](const nlohmann::json& data) {
                    HumanModLoader.Load(data);
                });

                auto appearanceFolder = modPath / constants::appearanceFolder;
                ParseJsonFilesInPath(appearanceFolder, [&](const nlohmann::json& data) {
                    AppearanceModLoader.Load(data);
                });

                auto buildingsFolder = modPath / constants::buildingsFolder;
                ParseJsonFilesInPath(buildingsFolder, [&](const nlohmann::json& data) {
                    BuildingModLoader.Load(data);
                });

                auto itemsFolder = modPath / constants::itemsFolder;
                ParseJsonFilesInPath(itemsFolder, [&](const nlohmann::json& data) {
                    ItemModLoader.Load(data);
                });

                auto skinsFolder = modPath / constants::skinsFolder;
                ParseJsonFilesInPath(skinsFolder, [&](const nlohmann::json& data) {
                    SkinModLoader.Load(data);
                });

                auto helpguideFolder = modPath / constants::helpguideFolder;
                ParseJsonFilesInPath(helpguideFolder, [&](const nlohmann::json& data) {
                    HelpGuideModLoader.Load(data);
                });

                auto rawFolder = modPath / constants::rawFolder;
                ParseJsonFilesInPath(rawFolder, [&](const nlohmann::json& data) {
                    RawTableLoader.Reload(data);
                });

                auto blueprintFolder = modPath / constants::blueprintsFolder;
                ParseJsonFilesInPath(blueprintFolder, [&](const nlohmann::json& data) {
                    BlueprintModLoader.Load(data);
                });

                auto spawnsFolder = modPath / constants::spawnsFolder;
                ParseJsonFilesInPath(spawnsFolder, [&](const nlohmann::json& data) {
                    SpawnLoader.Load(modName, data);
                });
            }
            catch (const std::exception& e)
            {
                errorCallback(modName, e);
            }
        });
	}

	void PalMainLoader::LoadLanguageMods(const std::filesystem::path& path)
	{
		const auto& currentLanguage = LanguageModLoader.GetCurrentLanguage();

        PS::Log<LogLevel::Verbose>(STR("Loading language mods...\n"));
        auto globalLanguageFolder = path / "global";
        if (fs::exists(globalLanguageFolder))
        {
            ParseJsonFilesInPath(globalLanguageFolder, [&](nlohmann::json data) {
                LanguageModLoader.Load(data);
            });
        }

        auto translationLanguageFolder = path / currentLanguage;
		if (fs::exists(translationLanguageFolder))
		{
            ParseJsonFilesInPath(translationLanguageFolder, [&](nlohmann::json data) {
                LanguageModLoader.Load(data);
            });
		}

        PS::Log<LogLevel::Verbose>(STR("Finished loading language mods.\n"));
	}

    void PalMainLoader::LoadCustomEnums()
    {
        EnumLoader.Initialize();

        PS::Log<LogLevel::Verbose>(STR("Loading custom enums...\n"));
        IterateModsFolder([&](const fs::path& modPath, const fs::path::string_type& modName) {
            auto enumsFolder = modPath / constants::enumsFolder;
            ParseJsonFilesInPath(enumsFolder, [&](nlohmann::json data) {
                try
                {
                    EnumLoader.Load(data);
                }
                catch (const std::exception& e)
                {
                    PS::Log<LogLevel::Error>(STR("Failed to add custom enums from mod {} - {}\n"), modName, RC::to_generic_string(e.what()));
                }
            });
        });

        PS::Log<LogLevel::Verbose>(STR("Finished loading custom enums.\n"));
    }

    void PalMainLoader::IterateModsFolder(const std::function<void(const std::filesystem::path&, const std::filesystem::path::string_type&)>& callback)
    {
        auto cwd = fs::path(UE4SSProgram::get_program().get_working_directory()) / "Mods" / "PalSchema" / "mods";
        if (fs::exists(cwd))
        {
            for (const auto& entry : fs::directory_iterator(cwd)) {
                if (entry.is_directory())
                {
                    auto& path = entry.path();
                    auto folderName = path.stem().native();
                    callback(entry.path(), folderName);
                }
            }
        }
    }

    void PalMainLoader::ParseJsonFileInPath(const std::filesystem::path& path, const std::function<void(const nlohmann::json&)>& callback)
    {
        if (!fs::exists(path))
        {
            PS::Log<LogLevel::Error>(STR("Failed parsing mod file {} - file doesn't exist.\n"), path.native());
            return;
        }

        auto ignoreComments = path.extension() == ".jsonc";
        std::ifstream f(path);

        nlohmann::json data = nlohmann::json::parse(f, nullptr, true, ignoreComments);
        callback(data);
    }

    void PalMainLoader::ParseJsonFilesInPath(const std::filesystem::path& path, const std::function<void(const nlohmann::json&)>& callback)
    {
        if (!fs::is_directory(path))
        {
            return;
        }

        for (const auto& file : fs::directory_iterator(path)) {
            try
            {
                auto filePath = file.path();
                if (filePath.has_extension())
                {
                    if (filePath.extension() == ".json" || filePath.extension() == ".jsonc")
                    {
                        ParseJsonFileInPath(filePath, callback);
                    }
                }
            }
            catch (const std::exception& e)
            {
                PS::Log<RC::LogLevel::Error>(STR("Failed parsing mod file {} - {}.\n"), RC::to_generic_string(file.path().native()), RC::to_generic_string(e.what()));
                throw std::runtime_error(e.what());
            }
        }
    }

    void PalMainLoader::PostLoad(UClass* This)
    {
        PostLoad_Hook.call(This);

        for (auto& Callback : PostLoadCallbacks)
        {
            Callback(This);
        }
    }

    // This entire function block will get called twice, it's fine.
    void PalMainLoader::GetPakFolders(const TCHAR* CmdLine, TArray<FString>* OutPakFolders)
    {
        PS::Log<LogLevel::Verbose>(STR("Calling original FPakPlatformFile::GetPakFolders...\n"));
        GetPakFolders_Hook.call(CmdLine, OutPakFolders);

        try
        {
            // Calling this here, because we want GMalloc to be available ASAP inside this hook so we can make our changes to the TArray.
            // Once UE4SS starts running things on Game Thread, this could be moved to PreInitialize.
            // Just for clarity, there is a check inside InitializeGMalloc to prevent it from running twice since GetPakFolders runs twice.
            UnrealOffsets::InitializeGMalloc();
        }
        catch (const std::exception& e)
        {
            PS::Log<LogLevel::Error>(STR("Failed to initialize GMalloc early: {}\n"), RC::to_generic_string(e.what()));
            PS::Log<LogLevel::Error>(STR("PalSchema won't be able to load paks from the PalSchema/mods folder.\n"));
            return;
        }
        
        PS::Log<LogLevel::Verbose>(STR("Preparing to add extra .pak read directory...\n"));
        auto ModsFolderPath = fs::path(UE4SSProgram::get_program().get_working_directory()) / "Mods" / "PalSchema" / "mods";
        auto AbsolutePath = ModsFolderPath.native();
        auto AbsolutePathWithSuffix = std::format(STR("{}/"), RC::to_generic_string(AbsolutePath));

        PS::Log<LogLevel::Verbose>(STR("Setting extra .pak read directory to {}\n"), AbsolutePathWithSuffix);

        // If GMalloc isn't properly initialized, accessing the TArray will crash.
        OutPakFolders->Add(FString(AbsolutePathWithSuffix.c_str()));

        PS::Log<LogLevel::Verbose>(STR("Added extra .pak read directory at {}\n"), AbsolutePathWithSuffix);
    }

    void PalMainLoader::OnDataTableSerialized(RC::Unreal::UDataTable* This, RC::Unreal::FArchive* Archive)
    {
        DatatableSerialize_Hook.call(This, Archive);

        for (auto& Callback : DatatableSerializeCallbacks)
        {
            Callback(This);
        }
    }

    void PalMainLoader::OnGameInstanceInit(RC::Unreal::UObject* This)
    {
        GameInstanceInit_Hook.call(This);

        for (auto& Callback : GameInstanceInitCallbacks)
        {
            Callback(This);
        }
    }

    RC::Unreal::UObject* PalMainLoader::StaticItemDataTable_Get(UPalStaticItemDataTable* This, FName ItemId)
    {
        auto StaticItemData = StaticItemDataTable_Get_Hook.call<UObject*>(This, ItemId);
        if (StaticItemData)
        {
            return StaticItemData;
        }

        // Some callers pass in an ItemId of NONE, but actually handle nullptr properly, so it is safe to pass the return back here.
        if (ItemId == RC::Unreal::NAME_None)
        {
            return StaticItemData;
        }

        if (ItemId.ToString().starts_with(STR("SkillUnlock_")))
        {
            return StaticItemData;
        }

        // If we got to this point, that means we actually have a nullptr to some item that used to exist, which means it is not safe to return nullptr.
        // Instead, we'll generate a dummy item for that ItemId and return it.
        // Subsequent calls to this hook with the same ItemId will just return in the first if block since it was added to StaticItemDataAsset.
        return PalItemModLoader::AddDummyItem(This, ItemId);
    }

    void PalMainLoader::UWorld_CleanupWorld(UWorld* This, bool bSessionEnded, bool bCleanupResources, UWorld* NewWorld)
    {
        WorldCleanUp_Hook.call(This, bSessionEnded, bCleanupResources, NewWorld);
        for (auto& Callback : WorldCleanUp_Callbacks)
        {
            Callback(This);
        }
    }
}
