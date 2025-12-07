#pragma once

#include <vector>
#include <functional>
#include "Loader/PalMonsterModLoader.h"
#include "Loader/PalHumanModLoader.h"
#include "Loader/PalLanguageModLoader.h"
#include "Loader/PalItemModLoader.h"
#include "Loader/PalSkinModLoader.h"
#include "Loader/PalAppearanceModLoader.h"
#include "Loader/PalBuildingModLoader.h"
#include "Loader/PalRawTableLoader.h"
#include "Loader/PalBlueprintModLoader.h"
#include "Loader/PalEnumLoader.h"
#include "Loader/PalHelpGuideModLoader.h"
#include "Loader/PalResourceLoader.h"
#include "Loader/PalSpawnLoader.h"
#include "FileWatch.hpp"

namespace RC::Unreal {
    class AGameModeBase;
    class UDataTable;
}

namespace UECustom {
    class UCompositeDataTable;
    class UWorldPartitionRuntimeLevelStreamingCell;
}

namespace Palworld {
    class UPalStaticItemDataTable;

	class PalMainLoader {
	public:
        PalMainLoader();

        ~PalMainLoader();

		void PreInitialize();

		void Initialize();

        // Should be called in Game Thread
        void ReloadMods();
	private:
		PalLanguageModLoader LanguageModLoader;
		PalMonsterModLoader MonsterModLoader;
		PalHumanModLoader HumanModLoader;
		PalItemModLoader ItemModLoader;
		PalSkinModLoader SkinModLoader;
		PalAppearanceModLoader AppearanceModLoader;
		PalBuildingModLoader BuildingModLoader;
		PalRawTableLoader RawTableLoader;
		PalBlueprintModLoader BlueprintModLoader;
        PalEnumLoader EnumLoader;
		PalHelpGuideModLoader HelpGuideModLoader;
        PalResourceLoader ResourceLoader;
        PalSpawnLoader SpawnLoader;

        std::unique_ptr<filewatch::FileWatch<std::wstring>> m_fileWatch;

        void HookDatatableSerialize();

        void HookStaticItemDataTable_Get();

        void HookWorldCleanup();

        void SetupAutoReload();

        // Makes PalSchema read paks from the 'PalSchema/mods' folder. Although the paks can be anywhere, prefer for them to be put inside 'YourModName/paks'.
        // This is intended for custom assets like models or textures that are part of a schema mod which makes it easier for modders to package their mod.
        // Requires a signature for FPakPlatformFile::GetPakFolders, otherwise this feature will not be available.
        void SetupAlternativePakPathReader();

        void InitCore();

        void InitLoaders();

        // errorCallback(modName, error) is fired each time a mod fails to load properly due to an error.
		void Load(const std::function<void(const std::filesystem::path::string_type&, const std::exception&)>& errorCallback);

		void LoadLanguageMods(const std::filesystem::path& path);

        void LoadCustomEnums();

        void IterateModsFolder(const std::function<void(const std::filesystem::path&, const std::filesystem::path::string_type&)>& callback);

        void ParseJsonFileInPath(const std::filesystem::path& path, const std::function<void(const nlohmann::json&)>& callback);

        void ParseJsonFilesInPath(const std::filesystem::path& path, const std::function<void(const nlohmann::json&)>& callback);
    private:
        static void PostLoad(RC::Unreal::UClass* This);

        static void GetPakFolders(const RC::Unreal::TCHAR* CmdLine, RC::Unreal::TArray<RC::Unreal::FString>* OutPakFolders);

        static void OnDataTableSerialized(RC::Unreal::UDataTable* This, RC::Unreal::FArchive* Archive);

        static void OnGameInstanceInit(RC::Unreal::UObject* This);

        static RC::Unreal::UObject* StaticItemDataTable_Get(UPalStaticItemDataTable* This, RC::Unreal::FName ItemId);

        static void UWorld_CleanupWorld(RC::Unreal::UWorld* This, bool bSessionEnded, bool bCleanupResources, RC::Unreal::UWorld* NewWorld);

        bool m_hasInit = false;

        static inline std::vector<std::function<void(RC::Unreal::UDataTable*)>> DatatableSerializeCallbacks;
        static inline std::vector<std::function<void(RC::Unreal::UObject*)>> GameInstanceInitCallbacks;
        static inline std::vector<std::function<void(RC::Unreal::UClass*)>> PostLoadCallbacks;
        static inline std::vector<std::function<void(RC::Unreal::UWorld*)>> WorldCleanUp_Callbacks;
        static inline std::vector<std::function<void()>> GetPakFoldersCallback;

        static inline SafetyHookInline DatatableSerialize_Hook;
        static inline SafetyHookInline GameInstanceInit_Hook;
        static inline SafetyHookInline PostLoad_Hook;
        static inline SafetyHookInline GetPakFolders_Hook;
        static inline SafetyHookInline StaticItemDataTable_Get_Hook;
        static inline SafetyHookInline WorldCleanUp_Hook;
	};
}