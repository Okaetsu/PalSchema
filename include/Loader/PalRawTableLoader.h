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

        void Apply(const RC::StringType& datatableName, RC::Unreal::UDataTable* datatable);

        void Apply(UECustom::UCompositeDataTable* compositeDatatable);

        void Apply(const nlohmann::json& data, RC::Unreal::UDataTable* table, LoadResult& outResult);
    protected:
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual void OnAutoReload(const RC::StringType& modName, const nlohmann::json& data) override final;

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual bool OnInitialize() override final;
        virtual void OnDatatableSerialized(RC::Unreal::UDataTable* datatable) override final;
    private:
        std::unordered_map<RC::StringType, std::vector<nlohmann::json>> m_tableDataMap;

        void HandleFilters(RC::Unreal::UDataTable* datatable, const nlohmann::json& data, LoadResult& outResult);

        void AddRow(RC::Unreal::UDataTable* datatable, const RC::Unreal::FName& rowName, const nlohmann::json& data, LoadResult& outResult);

        void EditRow(RC::Unreal::UDataTable* datatable, const RC::Unreal::FName& rowName, RC::Unreal::uint8* row, const nlohmann::json& data, LoadResult& outResult);

        void DeleteRow(RC::Unreal::UDataTable* datatable, const RC::Unreal::FName& rowName, LoadResult& outResult);

        void ModifyRowProperties(RC::Unreal::UDataTable* datatable, const RC::Unreal::FName& rowName, void* rowPtr, const nlohmann::json& data, LoadResult& outResult);

        void AddToTableDataMap(const std::string& datatableName, const nlohmann::json& data);
	};
}