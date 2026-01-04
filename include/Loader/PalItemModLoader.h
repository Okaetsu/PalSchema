#pragma once

#include "Loader/PalModLoaderBase.h"
#include "SDK/Classes/PalStaticItemDataAsset.h"

namespace RC::Unreal {
	class UDataTable;
}

namespace Palworld {
    class UPalStaticItemDataTable;

	class PalItemModLoader : public PalModLoaderBase {
	public:
		PalItemModLoader();

		~PalItemModLoader();

		void Initialize();

		virtual void Load(const nlohmann::json& Data) override final;
    public:
        static UPalStaticItemDataBase* AddDummyItem(UPalStaticItemDataTable* StaticItemDataTable, const RC::Unreal::FName& ItemId);
	private:
		void Add(const RC::Unreal::FName& ItemId, const nlohmann::json& Data);

		void Edit(const RC::Unreal::FName& ItemId, UPalStaticItemDataBase* Item, const nlohmann::json& Data);

		void AddRecipe(const RC::Unreal::FName& ItemId, const nlohmann::json& Recipe);

		void EditRecipe(const RC::Unreal::FName& ItemId, const nlohmann::json& Recipe);

		void AddTranslations(const RC::Unreal::FName& ItemId, const nlohmann::json& Data);

		void EditTranslations(const RC::Unreal::FName& ItemId, const nlohmann::json& Data);

        // Handles DT_ItemDataTable stuff
        void AddItemData(const RC::Unreal::FName& ItemId, const nlohmann::json& Data);

        void InitializeDummyTranslations();

		UPalStaticItemDataAsset* m_itemDataAsset{};
		RC::Unreal::UDataTable* m_itemDataTable{};
		RC::Unreal::UDataTable* m_itemRecipeTable{};
		RC::Unreal::UDataTable* m_nameTranslationTable{};
		RC::Unreal::UDataTable* m_descriptionTranslationTable{};
	};
}