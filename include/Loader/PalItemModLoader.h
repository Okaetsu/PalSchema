#pragma once

#include "Loader/PalModLoaderBase.h"
#include "SDK/Classes/PalStaticItemDataAsset.h"
#include "SDK/Classes/PalDynamicItemDataBase.h"
#include "SDK/Structs/FPalItemId.h"
#include "safetyhook.hpp"

namespace RC::Unreal {
	class UDataTable;
}

namespace Palworld {
    class UPalStaticItemDataTable;

	class PalItemModLoader : public PalModLoaderBase {
	public:
		PalItemModLoader();

		~PalItemModLoader();
    protected:
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual void OnAutoReload(const RC::StringType& modName, const std::filesystem::path& modFilePath) override final;

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual bool OnInitialize() override final;
	private:
        void LoadItems(const nlohmann::json& data);

		void Add(const RC::Unreal::FName& itemId, const nlohmann::json& data);

		void Edit(const RC::Unreal::FName& itemId, UPalStaticItemDataBase* item, const nlohmann::json& data);

		void AddRecipe(const RC::Unreal::FName& itemId, const nlohmann::json& recipe);

		void EditRecipe(const RC::Unreal::FName& itemId, const nlohmann::json& recipe);

		void AddTranslations(const RC::Unreal::FName& itemId, const nlohmann::json& data);

		void EditTranslations(const RC::Unreal::FName& itemId, const nlohmann::json& data);

        // Handles DT_ItemDataTable stuff
        void AddItemData(const RC::Unreal::FName& itemId, const nlohmann::json& data);

        void SetupHooks();

		UPalStaticItemDataAsset* m_itemDataAsset{};
		RC::Unreal::UDataTable* m_itemDataTable{};
		RC::Unreal::UDataTable* m_itemRecipeTable{};
		RC::Unreal::UDataTable* m_nameTranslationTable{};
		RC::Unreal::UDataTable* m_descriptionTranslationTable{};
    private:
        static inline void* ApplyItemSaveDataAddress = nullptr;
        static inline void* ApplyDynamicItemSaveDataAddress = nullptr;
        static inline SafetyHookInline UpdateItem_ServerInternalHook;
        static inline SafetyHookInline DynamicItemHook;

        static bool IsValidItem(RC::Unreal::UObject* worldContextObject, const RC::Unreal::FName& staticId);
        static void UpdateItem_Detour(RC::Unreal::UObject* self, FPalItemId* itemId, int amount, bool param4, bool param5);
        static UPalDynamicItemDataBase* CreateDynamicItemDatabase_Detour(RC::Unreal::UObject* self, FPalDynamicItemId* dynamicItemId, RC::Unreal::FName staticId, void* itemCreateParam);
	};
}