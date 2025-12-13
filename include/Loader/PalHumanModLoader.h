#pragma once

#include <set>
#include "Loader/PalModLoaderBase.h"
#include "nlohmann/json.hpp"

namespace RC::Unreal {
    class UDataTable;
}

namespace Palworld {
	class PalHumanModLoader : public PalModLoaderBase {
	public:
		PalHumanModLoader();

		~PalHumanModLoader();

		void Initialize();

		virtual void Load(const nlohmann::json& json) override final;
    private:
		void Add(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

        void Edit(uint8_t* TableRow, const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

		void AddBlueprint(const RC::Unreal::FName& CharacterId, const RC::StringType& BlueprintPath);

		void AddIcon(const RC::Unreal::FName& CharacterId, const RC::StringType& IconPath);

		void AddLoot(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

		void AddTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data);

        void EditTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data);

		void AddShop(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

		RC::Unreal::UDataTable* m_dataTable = nullptr;
		RC::Unreal::UDataTable* m_iconDataTable = nullptr;
		RC::Unreal::UDataTable* m_palBpClassTable = nullptr;
		RC::Unreal::UDataTable* m_dropItemTable = nullptr;
		RC::Unreal::UDataTable* m_npcNameTable = nullptr;
        RC::Unreal::UDataTable* m_palShortDescTable = nullptr;
        RC::Unreal::UDataTable* m_palLongDescTable = nullptr;
		RC::Unreal::UDataTable* m_npcTalkFlowTable = nullptr;
		RC::Unreal::UDataTable* m_ItemShopLotteryDataTable = nullptr;
		RC::Unreal::UDataTable* m_ItemShopCreateDataTable = nullptr;
		RC::Unreal::UDataTable* m_ItemShopSettingDataTable = nullptr;
	};
}