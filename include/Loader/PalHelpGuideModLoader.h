#pragma once

#include "Loader/PalModLoaderBase.h"
#include "SDK/Classes/PalNoteDataAsset.h"
#include "SDK/Classes/UDataTable.h"

namespace Palworld {
	class PalHelpGuideModLoader : public PalModLoaderBase {
	public:
		PalHelpGuideModLoader();

		~PalHelpGuideModLoader();

		void Initialize();

		virtual void Load(const nlohmann::json& Data) override final;
	private:
		UPalNoteDataAsset* m_helpGuideDataAsset{};
		UECustom::UDataTable* m_helpGuideMasterDataTable{};
		UECustom::UDataTable* m_helpGuideDescTextTable{};
		UECustom::UDataTable* m_helpGuideTextureDataTable{};

		void Add(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void Edit(const RC::Unreal::FName& NoteId, UPalNoteData* NoteData, const nlohmann::json& Data);

		void AddOrEditMasterData(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void AddOrEditDescText(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void AddOrEditTextureData(const RC::Unreal::FName& NoteId, const nlohmann::json& Data);

		void DeleteRelatedData(const RC::Unreal::FName& NoteId);
	};
}
