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
        void LoadItems(const nlohmann::json& Data);

		void Add(const RC::Unreal::FName& ItemId, const nlohmann::json& Data);

		void Edit(const RC::Unreal::FName& ItemId, UPalStaticItemDataBase* Item, const nlohmann::json& Data);

		void AddRecipe(const RC::Unreal::FName& ItemId, const nlohmann::json& Recipe);

		void EditRecipe(const RC::Unreal::FName& ItemId, const nlohmann::json& Recipe);

		void AddTranslations(const RC::Unreal::FName& ItemId, const nlohmann::json& Data);

		void EditTranslations(const RC::Unreal::FName& ItemId, const nlohmann::json& Data);

        // Handles DT_ItemDataTable stuff
        void AddItemData(const RC::Unreal::FName& ItemId, const RC::Unreal::FString& Type, const nlohmann::json& Data);

        void SetupHooks();

        bool IsCustomProperty(const std::string& Key);

		RC::Unreal::UDataTable* m_itemDataTable{};
		RC::Unreal::UDataTable* m_itemRecipeTable{};
		RC::Unreal::UDataTable* m_nameTranslationTable{};
		RC::Unreal::UDataTable* m_descriptionTranslationTable{};
    private:
        // DA_StaticItemDataAsset doesn't seem to ever unload, so it should be safe to store it in a static global.
        static inline UPalStaticItemDataAsset* GItemDataAsset{};

        static inline void* ApplyItemSaveDataAddress = nullptr;
        static inline void* ApplyDynamicItemSaveDataAddress = nullptr;
        static inline void* CraftItemCount_ApplyDataMapReturnAddress = nullptr;
        static inline SafetyHookInline UpdateItem_ServerInternalHook;
        static inline SafetyHookInline DynamicItemHook;
        static inline SafetyHookInline ValidateWorldSaveDynamicItemStaticIdsHook;
        static inline SafetyHookInline ValidateDynamicItemSaveDataHook;
        static inline SafetyHookInline ApplyDataMapHook;

        static bool IsValidItem(RC::Unreal::UObject* worldContextObject, const RC::Unreal::FName& staticId);
        static void UpdateItem_Detour(RC::Unreal::UObject* self, FPalItemId* ItemId, int amount, bool param4, bool param5);
        static UPalDynamicItemDataBase* CreateDynamicItemDatabase_Detour(RC::Unreal::UObject* self, FPalDynamicItemId* dynamicItemId, RC::Unreal::FName staticId, void* itemCreateParam);
        static bool ValidateWorldSaveDynamicItemStaticIds(RC::Unreal::UObject* idk, RC::Unreal::UObject* SaveGame, RC::Unreal::FString& idk3, RC::Unreal::FString& idk4);
        static bool ValidateDynamicItemSaveData(void* idk, RC::Unreal::UObject* DynamicItemDataBase, RC::Unreal::UObject* ItemIDManager, const RC::StringType& idk2);
        static void FPalPlayerRecordDataRepInfoArrayThreadSafe_IntVal_ApplyDataMap(void* self, const RC::Unreal::TMap<RC::Unreal::FName, RC::Unreal::int32>& MapToApply);
	};
}