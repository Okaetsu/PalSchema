#pragma once

#include "Loader/PalModLoaderBase.h"
#include "nlohmann/json.hpp"

namespace Palworld {
	class PalHumanModLoader : public PalModLoaderBase {
	public:
		PalHumanModLoader();

		~PalHumanModLoader();

		void Initialize();

		virtual void Load(const nlohmann::json& json) override final;

		void Add(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

        void Edit(uint8_t* TableRow, const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

		void AddBlueprint(const RC::Unreal::FName& CharacterId, const RC::StringType& BlueprintPath);

		void AddIcon(const RC::Unreal::FName& CharacterId, const RC::StringType& IconPath);

		void AddLoot(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

		void AddTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data);

        void EditTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data);

		UECustom::UDataTable* n_dataTable;
		UECustom::UDataTable* n_iconDataTable;
		UECustom::UDataTable* n_palBpClassTable;
		UECustom::UDataTable* n_dropItemTable;
		UECustom::UDataTable* n_npcNameTable;
        UECustom::UDataTable* n_palShortDescTable;
        UECustom::UDataTable* n_palLongDescTable;
	};
}