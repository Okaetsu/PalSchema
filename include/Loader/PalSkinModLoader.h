#pragma once

#include "Loader/PalModLoaderBase.h"

namespace RC::Unreal {
	class UObject;
    class UDataTable;
}

namespace UECustom {
	class UDataAsset;
}

namespace Palworld {
	class PalSkinModLoader : public PalModLoaderBase {
	public:
		PalSkinModLoader();

		~PalSkinModLoader();
    protected:
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual void OnAutoReload(const RC::StringType& modName, const std::filesystem::path& modFilePath) override final;

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual bool OnInitialize() override final;
	private:
		UECustom::UDataAsset* m_skinDataAsset{};
		RC::Unreal::UDataTable* m_skinIconTable{};
		RC::Unreal::UDataTable* m_skinTranslationTable{};

		std::unordered_map<RC::StringType, RC::Unreal::UClass*> m_palSkinDataBaseClassCache;

        void LoadSkins(const nlohmann::json& data);

		void Add(const RC::Unreal::FName& SkinId, RC::Unreal::TMap<RC::Unreal::FName, RC::Unreal::UObject*>* StaticSkinMap, const nlohmann::json& Data);

		void Edit(const RC::Unreal::FName& SkinId, RC::Unreal::UObject* Item, const nlohmann::json& Data);

		void AddTranslation(const RC::Unreal::FName& SkinId, const nlohmann::json& Data);

		void EditTranslation(const RC::Unreal::FName& SkinId, const nlohmann::json& Data);

		RC::Unreal::UClass* LoadOrCacheSkinDataBaseClass(const RC::StringType& Path);
	};
}