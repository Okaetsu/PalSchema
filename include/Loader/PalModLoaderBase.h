#pragma once

#include "Unreal/NameTypes.hpp"
#include "SDK/Classes/Custom/UDataTableStore.h"
#include "nlohmann/json.hpp"
#include <string>

namespace RC::Unreal {
	class UDataTable;
}

namespace Palworld {
    enum class EEngineLifecyclePhase {
        PreEngineInit,
        PostEngineInit,
        UE4SSInit,
        GameInstanceInit,
    };

	class PalModLoaderBase {
	public:
		virtual ~PalModLoaderBase();
        
        void AssignDatatableRegistry(UECustom::UDataTableRegistry& datatableRegistry);

        const RC::StringType& GetDisplayName() const;

        void Setup();
        void AutoReload(const RC::StringType& modName, const std::filesystem::path& modFilePath);
        void Load(const std::filesystem::path& modPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase);

		void Initialize(const EEngineLifecyclePhase& engineLifecyclePhase);

        const bool& HasInitialized() const;

        // Returns the folder name specified for this loader, e.g. blueprints, raw, pals, etc.
        const std::string& GetModFolderType();
    protected:
        PalModLoaderBase(const std::string& modFolderName);

        void SetDisplayName(const RC::StringType& displayName);

        void IterateModsFolder(const std::function<void(const std::filesystem::path&, const RC::StringType&)>& callback);

        // Does not throw if the data table isn't found, returns nullptr
        RC::Unreal::UDataTable* TryGetDatatableByName(const std::string& name);

        // Throws an error if data table isn't found
        RC::Unreal::UDataTable* GetDatatableByName(const std::string& name);
    protected:
        // Called when the Loader is created, calling UE functions during Setup is not safe
        virtual void OnSetup();
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase);
        virtual void OnAutoReload(const RC::StringType& modName, const std::filesystem::path& modFilePath);

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) = 0;

        // You should return true or false depending on if the initialization was successful
        virtual bool OnInitialize() = 0;

        // Do post initialize logic here like loading mods, etc.
        virtual void PostInitialize();

        virtual void OnDatatableSerialized(RC::Unreal::UDataTable* datatable);
    private:
        void Initialize_Internal();

        std::string m_modFolderType = "";
        RC::StringType m_displayName = TEXT("Unknown Loader");
        UECustom::UDataTableRegistry* m_datatableRegistry = nullptr;
        bool m_hasInitialized = false;
        std::mutex m_mutex;

        UECustom::DatatableSerializeCallbackId m_datatableSerializeCallbackId{};
	};
}