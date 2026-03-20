#pragma once

#include "Loader/PalModLoaderBase.h"
#include "SDK/Classes/PalNoteDataAsset.h"

namespace RC::Unreal {
    class UDataTable;
}

namespace Palworld {
	class PalHelpGuideModLoader : public PalModLoaderBase {
	public:
		PalHelpGuideModLoader();

		~PalHelpGuideModLoader();
    protected:
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual void OnAutoReload(const RC::StringType& modName, const std::filesystem::path& modFilePath) override final;

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual bool OnInitialize() override final;
	private:
		UPalNoteDataAsset* m_helpGuideDataAsset{};
		RC::Unreal::UDataTable* m_helpGuideDescTextTable{};

        void LoadHelpGuides(const nlohmann::json& data);

		void Add(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void Edit(const RC::Unreal::FName& NoteId, UPalNoteData* NoteData, const nlohmann::json& Data);

		void AddOrEditDescText(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void DeleteRelatedData(const RC::Unreal::FName& NoteId);
	};
}
