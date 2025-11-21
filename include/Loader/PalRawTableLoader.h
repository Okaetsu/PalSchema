#pragma once

#include "Unreal/NameTypes.hpp"
#include "Loader/PalModLoaderBase.h"
#include "nlohmann/json.hpp"

namespace RC::Unreal {
    class UDataTable;
}

namespace UECustom {
	class UCompositeDataTable;
}

namespace Palworld {
	class PalRawTableLoader : public PalModLoaderBase {
        struct LoadResult {
            int SuccessfulModifications = 0;
            int SuccessfulAdditions = 0;
            int SuccessfulDeletions = 0;
            int ErrorCount = 0;
        };
	public:
		PalRawTableLoader();

		~PalRawTableLoader();

		void Initialize();

        void OnDataTableChanged(RC::Unreal::UDataTable* Table);

        void Apply(const RC::StringType& TableName, RC::Unreal::UDataTable* Table);

        void Apply(UECustom::UCompositeDataTable* Table);

        void Apply(const nlohmann::json& Data, RC::Unreal::UDataTable* Table, LoadResult& OutResult);

		virtual void Load(const nlohmann::json& Data) override final;

        void Reload(const nlohmann::json& Data);
    private:
        std::unordered_map<RC::StringType, std::vector<nlohmann::json>> m_tableDataMap;
        std::unordered_map<RC::StringType, RC::Unreal::UDataTable*> m_tableMap;

        void HandleFilters(RC::Unreal::UDataTable* Table, const nlohmann::json& Data, LoadResult& OutResult);

        void AddRow(RC::Unreal::UDataTable* Table, const RC::Unreal::FName& RowName, const nlohmann::json& Data, LoadResult& OutResult);

        void EditRow(RC::Unreal::UDataTable* Table, const RC::Unreal::FName& RowName, RC::Unreal::uint8* Row, const nlohmann::json& Data, LoadResult& OutResult);

        void DeleteRow(RC::Unreal::UDataTable* Table, const RC::Unreal::FName& RowName, LoadResult& OutResult);

        void ModifyRowProperties(RC::Unreal::UDataTable* Table, const RC::Unreal::FName& RowName, void* RowPtr, const nlohmann::json& Data, LoadResult& OutResult);

        void AddToTableDataMap(const std::string& TableName, const nlohmann::json& Data);
	};
}