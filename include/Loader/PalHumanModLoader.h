#pragma once

#include <set>
#include "Loader/PalModLoaderBase.h"
#include "nlohmann/json.hpp"
#include "SDK/Structs/FVector.h"

namespace RC::Unreal {
    class UDataTable;
}

namespace UECustom {
    class UWorldPartitionRuntimeLevelStreamingCell;
}

namespace Palworld {
    struct PalHumanSpawnParams {
        RC::Unreal::FVector Location;
        RC::Unreal::FName CharacterId;
    };

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

		void AddSpawn(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

		void AddTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data);

        void EditTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data);

		void AddShop(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

        // This is called whenever a world partition is loaded within the main world.
        void OnCellLoaded(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell);

        void SpawnNPC(UECustom::UWorldPartitionRuntimeLevelStreamingCell* cell, const PalHumanSpawnParams& params);

        std::vector<PalHumanSpawnParams> m_spawns;
        std::set<UECustom::FVector> m_occupiedLocations;

		RC::Unreal::UDataTable* n_dataTable = nullptr;
		RC::Unreal::UDataTable* n_iconDataTable = nullptr;
		RC::Unreal::UDataTable* n_palBpClassTable = nullptr;
		RC::Unreal::UDataTable* n_dropItemTable = nullptr;
		RC::Unreal::UDataTable* n_npcNameTable = nullptr;
        RC::Unreal::UDataTable* n_palShortDescTable = nullptr;
        RC::Unreal::UDataTable* n_palLongDescTable = nullptr;
		RC::Unreal::UDataTable* n_npcTalkFlowTable = nullptr;
		RC::Unreal::UDataTable* n_ItemShopLotteryDataTable = nullptr;
		RC::Unreal::UDataTable* n_ItemShopCreateDataTable = nullptr;
		RC::Unreal::UDataTable* n_ItemShopSettingDataTable = nullptr;
	};
}