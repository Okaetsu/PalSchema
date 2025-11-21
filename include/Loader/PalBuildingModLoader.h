#pragma once

#include "Loader/PalModLoaderBase.h"
#include "Unreal/NameTypes.hpp"

namespace RC::Unreal {
	class UObject;
    class UDataTable;
}

namespace UECustom {
	class UDataAsset;
}

namespace Palworld {
	class PalBuildingModLoader : public PalModLoaderBase {
	public:
		PalBuildingModLoader();

		~PalBuildingModLoader();

		void Initialize();

		virtual void Load(const nlohmann::json& json) override final;
	private:
		RC::Unreal::UDataTable* m_mapObjectAssignData{};
		RC::Unreal::UDataTable* m_mapObjectFarmCrop{};
		RC::Unreal::UDataTable* m_mapObjectItemProductDataTable{};
		RC::Unreal::UDataTable* m_mapObjectMasterDataTable{};
		RC::Unreal::UDataTable* m_mapObjectNameTable{};
		RC::Unreal::UDataTable* m_buildObjectDataTable{};
		RC::Unreal::UDataTable* m_buildObjectIconDataTable{};
        RC::Unreal::UDataTable* m_buildObjectDescTable{};
		RC::Unreal::UDataTable* m_technologyRecipeUnlockTable{};
		RC::Unreal::UDataTable* m_technologyNameTable{};
		RC::Unreal::UDataTable* m_technologyDescTable{};

		void Add(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

		void Edit(uint8_t* ExistingRow, const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

		void SetupBuildData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

		void SetupIconData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

		void SetupAssignments(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

        void SetupAssignment(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

		void SetupCropData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

		void SetupItemProductData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

		void SetupTechnologyData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

        void SetupTranslations(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data);

        void SetupTranslation(const RC::StringType& RowKey, RC::Unreal::UDataTable* DataTable, const nlohmann::json& Value);

        void ImportJson(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data, RC::Unreal::UDataTable* DataTable);

        RC::StringType GetAssignIDSuffixByWorkType(const std::string& WorkType);
    private:
        static inline int RadialIndex = 1000;

        static int GetNextRadialIndex();
	};
}