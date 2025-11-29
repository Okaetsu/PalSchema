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

		void Initialize();

		virtual void Load(const nlohmann::json& Data) override final;
	private:
		UPalNoteDataAsset* m_helpGuideDataAsset{};
		RC::Unreal::UDataTable* m_helpGuideMasterDataTable{};
		RC::Unreal::UDataTable* m_helpGuideDescTextTable{};
		RC::Unreal::UDataTable* m_helpGuideTextureDataTable{};

		void Add(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void Edit(const RC::Unreal::FName& NoteId, UPalNoteData* NoteData, const nlohmann::json& Data);

		void AddOrEditMasterData(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void AddOrEditDescText(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void DeleteRelatedData(const RC::Unreal::FName& NoteId);
	};
}
