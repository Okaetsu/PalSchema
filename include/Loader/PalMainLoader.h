#pragma once

#include <vector>
#include <functional>
#include "Loader/PalResourceLoader.h"
#include "SDK/Classes/Custom/UDataTableStore.h"
#include "FileWatch.hpp"
#include "safetyhook.hpp"

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
	private:
        std::vector<std::unique_ptr<PalModLoaderBase>> m_loaders;

        std::unique_ptr<filewatch::FileWatch<std::wstring>> m_fileWatch;

        UECustom::UDataTableRegistry m_datatableRegistry;

        void IterateModsFolder(const std::function<void(const std::filesystem::path&, const RC::StringType&)>& callback);

        void SetupPostEngineInitLoaders();

        void SetupGameInstanceInitLoaders();

        void HookDatatableSerialize();

        void HookStaticItemDataTable_Get();

        void HookGameInstanceInit();

        void CreateLoaders();

        void SetupAutoReload();

        // Makes PalSchema read paks from the 'PalSchema/mods' folder. Although the paks can be anywhere, prefer for them to be put inside 'YourModName/paks'.
        // This is intended for custom assets like models or textures that are part of a schema mod which makes it easier for modders to package their mod.
        // Requires a signature for FPakPlatformFile::GetPakFolders, otherwise this feature will not be available.
        void SetupAlternativePakPathReader();

        void InitCore();

        void RegisterLoader(std::unique_ptr<PalModLoaderBase> newLoader);

        void InitializeMods(EEngineLifecyclePhase engineLifecyclePhase);
        void LoadMods(EEngineLifecyclePhase engineLifecyclePhase);
    private:
        static std::filesystem::path GetModsPath();

        static void GetPakFolders(const RC::Unreal::TCHAR* CmdLine, RC::Unreal::TArray<RC::Unreal::FString>* OutPakFolders);

        static void OnDataTableSerialized(RC::Unreal::UDataTable* This, RC::Unreal::FArchive* Archive);

        static void OnGameInstanceInit(RC::Unreal::UObject* This);

        static RC::Unreal::UObject* StaticItemDataTable_Get(UPalStaticItemDataTable* This, RC::Unreal::FName ItemId);

        bool m_hasInit = false;

        static inline std::vector<std::function<void(RC::Unreal::UDataTable*)>> DatatableSerializeCallbacks;
        static inline std::vector<std::function<void(RC::Unreal::UObject*)>> GameInstanceInitCallbacks;
        static inline std::vector<std::function<void()>> GetPakFoldersCallback;

        static inline SafetyHookInline DatatableSerialize_Hook;
        static inline SafetyHookInline GameInstanceInit_Hook;
        static inline SafetyHookInline GetPakFolders_Hook;
        static inline SafetyHookInline StaticItemDataTable_Get_Hook;
	};
}