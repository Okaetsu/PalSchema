#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "SDK/Classes/PalItemIDManager.h"
#include "SDK/Classes/PalStaticItemDataTable.h"
#include "SDK/Classes/PalStaticArmorItemData.h"
#include "SDK/Classes/PalStaticConsumeItemData.h"
#include "SDK/Classes/PalStaticWeaponItemData.h"
#include "SDK/Classes/PalDynamicWeaponItemDataBase.h"
#include "SDK/Classes/PalDynamicArmorItemDataBase.h"
#include "SDK/Classes/PalUtility.h"
#include "SDK/Structs/FPalCharacterIconDataRow.h"
#include "SDK/Structs/Custom/FManagedStruct.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Helpers/String.hpp"
#include "Utility/Logging.h"
#include "Utility/JsonHelpers.h"
#include "Loader/PalItemModLoader.h"
#include <SDK/Structs/FPalLocalizedTextData.h>
#include <SDK/PalSignatures.h>

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalItemModLoader::PalItemModLoader() : PalModLoaderBase("items") {
        SetDisplayName(TEXT("Item Loader"));
    }

	PalItemModLoader::~PalItemModLoader() {}

	void PalItemModLoader::OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase)
	{
        if (engineLifecyclePhase != EEngineLifecyclePhase::GameInstanceInit)
        {
            return;
        }

        PS::JsonHelpers::ParseJsonFilesInPath(loaderPath, [&](const nlohmann::json& data) {
            LoadItems(data);
        });
	}

    void PalItemModLoader::OnAutoReload(const RC::StringType& modName, const std::filesystem::path& modFilePath)
    {
        PS::JsonHelpers::ParseJsonFileInPath(modFilePath, [&](const nlohmann::json& data) {
            LoadItems(data);
        });
    }

    bool PalItemModLoader::CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase == EEngineLifecyclePhase::GameInstanceInit)
        {
            return true;
        }

        return false;
    }

    bool PalItemModLoader::OnInitialize()
    {
        try
        {
            m_itemDataAsset = UECustom::UObjectGlobals::StaticFindObject<UPalStaticItemDataAsset*>(nullptr, nullptr,
                STR("/Game/Pal/DataAsset/Item/DA_StaticItemDataAsset.DA_StaticItemDataAsset"));

            m_itemDataTable = GetDatatableByName("DT_ItemDataTable");
            m_itemRecipeTable = GetDatatableByName("DT_ItemRecipeDataTable");
            m_nameTranslationTable = GetDatatableByName("DT_ItemNameText");
            m_descriptionTranslationTable = GetDatatableByName("DT_ItemDescriptionText");

            SetupHooks();
        }
        catch (const std::exception& e)
        {
            PS::Log<LogLevel::Error>(STR("Unable to initialize {}, {}\n"), GetDisplayName(), RC::to_generic_string(e.what()));
            return false;
        }

        return true;
    }

    void PalItemModLoader::LoadItems(const nlohmann::json& data)
    {
        for (auto& [Key, Value] : data.items())
        {
            auto ItemId = FName(RC::to_generic_string(Key), FNAME_Add);
            auto Row = m_itemDataAsset->StaticItemDataMap.Find(ItemId);
            if (Value.is_null())
            {
                if (!Row) return;
                m_itemDataAsset->StaticItemDataMap.Remove(ItemId);
                PS::Log<RC::LogLevel::Normal>(STR("Deleted Item '{}'\n"), ItemId.ToString());
            }
            else
            {
                if (Row)
                {
                    Edit(ItemId, *Row, Value);
                    PS::Log<RC::LogLevel::Normal>(STR("Modified Item '{}'\n"), ItemId.ToString());
                }
                else
                {
                    Add(ItemId, Value);
                    PS::Log<RC::LogLevel::Normal>(STR("Added Item '{}'\n"), ItemId.ToString());
                }
            }
        }
    }

	void PalItemModLoader::Add(const RC::Unreal::FName& ItemId, const nlohmann::json& Data)
	{
		if (ItemId == NAME_None)
		{
			throw std::runtime_error("ID was set to None");
		}

		if (!Data.contains("Type"))
		{
			throw std::runtime_error(std::format("You must supply a Type field in {} when adding new items", RC::to_string(ItemId.ToString())));
		}

		if (!Data.at("Type").is_string())
		{
			throw std::runtime_error(std::format("Type must be a string in {}", RC::to_string(ItemId.ToString())));
		}

		auto Type = Data.at("Type").get<std::string>();

		UClass* DatabaseClass = nullptr;
		UClass* DynamicDatabaseClass = nullptr;
		if (Type == "Armor" || Type == "PalStaticArmorItemData")
		{
			DatabaseClass = UPalStaticArmorItemData::StaticClass();
			DynamicDatabaseClass = UPalDynamicArmorItemDataBase::StaticClass();
		}
		else if (Type == "Weapon" || Type == "PalStaticWeaponItemData")
		{
			DatabaseClass = UPalStaticWeaponItemData::StaticClass();
			DynamicDatabaseClass = UPalDynamicWeaponItemDataBase::StaticClass();
		}
		else if (Type == "Consumable" || Type == "PalStaticConsumeItemData")
		{
			DatabaseClass = UPalStaticConsumeItemData::StaticClass();
		}
		else if (Type == "Generic" || Type == "PalStaticItemDataBase")
		{
			DatabaseClass = UPalStaticItemDataBase::StaticClass();
		}
		else
		{
			throw std::runtime_error(std::format("Type {} in {} isn't supported, must be Armor, Weapon, Consumable or Generic", Type, RC::to_string(ItemId.ToString())));
		}

		if (!DatabaseClass->GetPropertyByNameInChain(STR("ID")))
		{
			throw std::runtime_error("Property 'ID' has changed in DA_StaticItemDataAsset. Update to Pal Schema is needed.");
		}

		if (!DatabaseClass->GetPropertyByNameInChain(STR("DynamicItemDataClass")))
		{
			throw std::runtime_error("Property 'DynamicItemDataClass' has changed in DA_StaticItemDataAsset. Update to Pal Schema is needed.");
		}

		FStaticConstructObjectParameters ConstructParams(DatabaseClass, m_itemDataAsset);
		ConstructParams.Name = NAME_None;

		auto Item = UObjectGlobals::StaticConstructObject<UPalStaticItemDataBase*>(ConstructParams);

		auto IDProperty = Item->GetValuePtrByPropertyNameInChain<FName>(STR("ID"));
		if (IDProperty)
		{
			*IDProperty = ItemId;
		}

		auto DynamicItemDataClassProperty = Item->GetValuePtrByPropertyNameInChain<UClass*>(STR("DynamicItemDataClass"));
		if (DynamicItemDataClassProperty)
		{
			*DynamicItemDataClassProperty = DynamicDatabaseClass;
		}

        for (FProperty* Property : TFieldRange<FProperty>(DatabaseClass, EFieldIterationFlags::IncludeSuper))
        {
            auto PropertyName = RC::to_string(Property->GetName());
            if (PropertyName == "DynamicItemDataClass")
            {
                // We've already set this earlier so we skip it.
                continue;
            }
            if (Data.contains(PropertyName))
            {
                PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(Item), Property, Data.at(PropertyName));
            }
        }
		
		if (Data.contains("Recipe"))
		{
			AddRecipe(ItemId, Data.at("Recipe"));
		}

        AddItemData(ItemId, Data);

		AddTranslations(ItemId, Data);

		m_itemDataAsset->StaticItemDataMap.Add(ItemId, Item);
	}

	void PalItemModLoader::Edit(const RC::Unreal::FName& ItemId, UPalStaticItemDataBase* Item, const nlohmann::json& Data)
	{
        for (FProperty* Property : TFieldRange<FProperty>(Item->GetClassPrivate(), EFieldIterationFlags::IncludeSuper))
        {
            auto PropertyName = RC::to_string(Property->GetName());
            if (PropertyName == "DynamicItemDataClass")
            {
                continue;
            }
            if (PropertyName == "ID")
            {
                // Editing the ID is a bad idea, hence we skip it.
                continue;
            }
            if (Data.contains(PropertyName))
            {
                PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(Item), Property, Data.at(PropertyName));
            }
        }

		if (Data.contains("Recipe"))
		{
			EditRecipe(ItemId, Data.at("Recipe"));
		}

		EditTranslations(ItemId, Data);
	}

	void PalItemModLoader::AddRecipe(const RC::Unreal::FName& ItemId, const nlohmann::json& Recipe)
	{
		auto RowStruct = m_itemRecipeTable->GetRowStruct().Get();

		auto ItemRecipeData = FMemory::Malloc(RowStruct->GetStructureSize());
		RowStruct->InitializeStruct(ItemRecipeData);

		for (auto& [property_name, property_value] : Recipe.items())
		{
			auto KeyName = RC::to_generic_string(property_name);

			if (KeyName == STR("Product_Id"))
			{
				// We will set this later based on the key used for the json object, so we skip it for now.
				continue;
			}

			if (KeyName == STR("Editor_RowNameHash"))
			{
				// We don't need to change this due to it being editor related, skip.
				continue;
			}

			auto Property = RowStruct->GetPropertyByName(KeyName.c_str());
			if (Property)
			{
				try
				{
					PropertyHelper::CopyJsonValueToContainer(ItemRecipeData, Property, property_value);
				}
				catch (const std::exception& e)
				{
					FMemory::Free(ItemRecipeData);
					throw std::runtime_error(e.what());
				}
			}
		}

		auto ProductIdProperty = RowStruct->GetPropertyByName(STR("Product_Id"));
		if (ProductIdProperty)
		{
			FMemory::Memcpy(ProductIdProperty->ContainerPtrToValuePtr<void>(ItemRecipeData), &ItemId, sizeof(FName));
		}

		m_itemRecipeTable->AddRow(ItemId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(ItemRecipeData));

        PS::Log<LogLevel::Normal>(STR("Added new Recipe for Item '{}'.\n"), ItemId.ToString());
	}

	void PalItemModLoader::EditRecipe(const RC::Unreal::FName& ItemId, const nlohmann::json& Recipe)
	{
		auto RowStruct = m_itemRecipeTable->GetRowStruct().Get();

		auto RecipeRow = m_itemRecipeTable->FindRowUnchecked(ItemId);
		if (!RecipeRow)
		{
			throw std::runtime_error(std::format("Row for Recipe '{}' doesn't exist", RC::to_string(ItemId.ToString())));
		}

		for (auto& [property_name, property_value] : Recipe.items())
		{
			auto KeyName = RC::to_generic_string(property_name);
			if (KeyName == STR("Editor_RowNameHash"))
			{
				// We don't need to change this due to it being editor related, skip.
				continue;
			}

			auto Property = RowStruct->GetPropertyByName(KeyName.c_str());
			if (Property)
			{
				PropertyHelper::CopyJsonValueToContainer(RecipeRow, Property, property_value);
			}
		}

        PS::Log<LogLevel::Normal>(STR("Modified Recipe for Item '{}'.\n"), ItemId.ToString());
	}

	void PalItemModLoader::AddTranslations(const RC::Unreal::FName& ItemId, const nlohmann::json& Data)
	{
		if (Data.contains("Name"))
		{
			auto FixedItemId = std::format(STR("ITEM_NAME_{}"), ItemId.ToString());
			auto TranslationRowStruct = m_nameTranslationTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto TranslationRowData = FMemory::Malloc(TranslationRowStruct->GetStructureSize());
				TranslationRowStruct->InitializeStruct(TranslationRowData);

				try
				{
					PropertyHelper::CopyJsonValueToContainer(TranslationRowData, TextProperty, Data.at("Name"));
				}
				catch (const std::exception& e)
				{
					FMemory::Free(TranslationRowData);
					throw std::runtime_error(e.what());
				}

				m_nameTranslationTable->AddRow(FName(FixedItemId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(TranslationRowData));
			}
		}

		if (Data.contains("Description"))
		{
			auto FixedItemId = std::format(STR("ITEM_DESC_{}"), ItemId.ToString());
			auto TranslationRowStruct = m_descriptionTranslationTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto TranslationRowData = FMemory::Malloc(TranslationRowStruct->GetStructureSize());
				TranslationRowStruct->InitializeStruct(TranslationRowData);

				try
				{
					PropertyHelper::CopyJsonValueToContainer(TranslationRowData, TextProperty, Data.at("Description"));
				}
				catch (const std::exception& e)
				{
					FMemory::Free(TranslationRowData);
					throw std::runtime_error(e.what());
				}

				m_descriptionTranslationTable->AddRow(FName(FixedItemId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(TranslationRowData));
			}
		}
	}

	void PalItemModLoader::EditTranslations(const RC::Unreal::FName& ItemId, const nlohmann::json& Data)
	{
		if (Data.contains("Name"))
		{
			auto FixedItemId = std::format(STR("ITEM_NAME_{}"), ItemId.ToString());
			auto TranslationRowStruct = m_nameTranslationTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = m_nameTranslationTable->FindRowUnchecked(FName(FixedItemId, FNAME_Add));
				if (Row)
				{
					PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data.at("Name"));
				}
			}
		}

		if (Data.contains("Description"))
		{
			auto FixedItemId = std::format(STR("ITEM_DESC_{}"), ItemId.ToString());
			auto TranslationRowStruct = m_nameTranslationTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = m_descriptionTranslationTable->FindRowUnchecked(FName(FixedItemId, FNAME_Add));
				if (Row)
				{
					PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data.at("Description"));
				}
			}
		}
	}

    void PalItemModLoader::AddItemData(const RC::Unreal::FName& ItemId, const nlohmann::json& Data)
    {
        auto RowStruct = m_itemDataTable->GetRowStruct().Get();
        FManagedStruct RowData{ RowStruct };

        auto LegalProp = RowStruct->GetPropertyByName(STR("bLegalInGame"));
        if (!LegalProp)
        {
            PS::Log<LogLevel::Error>(STR("Property 'bLegalInGame' does not exist in DT_ItemDataTable, skipping addition of data for {}.\n"), ItemId.ToString());
            return;
        }

        if (Data.contains("bLegalInGame"))
        {
            PropertyHelper::CopyJsonValueToContainer(RowData.GetData(), LegalProp, Data.at("bLegalInGame"));
        }
        else
        {
            bool LegalValue = true;
            FMemory::Memcpy(LegalProp->ContainerPtrToValuePtr<void>(RowData.GetData()), &LegalValue, sizeof(bool));
        }

        m_itemDataTable->AddRow(ItemId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(RowData.GetData()));
    }

    void PalItemModLoader::SetupHooks()
    {
        try
        {
            auto address = Palworld::SignatureManager::GetSignature("UPalItemSlot::UpdateItem_ServerInternal");
            if (!address)
            {
                throw std::runtime_error("Signature for UPalItemSlot::UpdateItem_ServerInternal could not be found");
            }

            ApplyItemSaveDataAddress = Palworld::SignatureManager::GetSignature("UPalItemContainer::ApplySaveData");
            if (!ApplyItemSaveDataAddress)
            {
                throw std::runtime_error("Signature for UPalItemContainer::ApplySaveData could not be found");
            }

            auto address2 = Palworld::SignatureManager::GetSignature("UPalDynamicItemWorldSubsystem::Create_ServerInternal");
            if (!address2)
            {
                throw std::runtime_error("Signature for UPalDynamicItemWorldSubsystem::Create_ServerInternal could not be found");
            }

            ApplyDynamicItemSaveDataAddress = Palworld::SignatureManager::GetSignature("UPalDynamicItemWorldSubsystem::ApplyWorldSaveData");
            if (!ApplyDynamicItemSaveDataAddress)
            {
                throw std::runtime_error("Signature for UPalDynamicItemWorldSubsystem::ApplyWorldSaveData could not be found");
            }

            UpdateItem_ServerInternalHook = safetyhook::create_inline(reinterpret_cast<void*>(address),
                UpdateItem_Detour);

            DynamicItemHook = safetyhook::create_inline(reinterpret_cast<void*>(address2),
                CreateDynamicItemDatabase_Detour);
        }
        catch (const std::exception& e)
        {
            PS::Log<LogLevel::Error>(STR("{}. PalSchema will be unable to clean up invalid items and worlds with invalid items will crash on load.\n"), 
                                     RC::to_generic_string(e.what()));
        }

    }

    bool PalItemModLoader::IsValidItem(RC::Unreal::UObject* worldContextObject, const RC::Unreal::FName& staticId)
    {
        auto itemIdManager = UPalUtility::GetItemIDManager(worldContextObject);
        auto staticItemData = itemIdManager->GetStaticItemData(staticId);
        if (staticItemData)
        {
            return true;
        }
        
        return false;
    }

    void PalItemModLoader::UpdateItem_Detour(RC::Unreal::UObject* self, FPalItemId* itemId, int amount, bool param4, bool param5)
    {
        if (_ReturnAddress() == ApplyItemSaveDataAddress && !IsValidItem(self, itemId->StaticId))
        {
            PS::Log<LogLevel::Warning>(STR("Item '{}' is invalid. Deleting.\n"), itemId->StaticId.ToString());
            itemId->StaticId = NAME_None;
            amount = 0;
        }

        UpdateItem_ServerInternalHook.call(self, itemId, amount, param4, param5);
    }

    UPalDynamicItemDataBase* PalItemModLoader::CreateDynamicItemDatabase_Detour(RC::Unreal::UObject* self, FPalDynamicItemId* dynamicItemId, RC::Unreal::FName staticId, void* itemCreateParam)
    {
        if (_ReturnAddress() == ApplyDynamicItemSaveDataAddress && !IsValidItem(self, staticId))
        {
            PS::Log<LogLevel::Warning>(STR("Item '{}' is invalid. Dynamic data for this item will be deleted on next save.\n"), staticId.ToString());

            // Shouldn't matter what we use as the staticId as long as it meets the two conditions:
            // 1. Must exist in the vanilla game
            // 2. Has a DynamicItemDataBase (Weapon, Armor, Egg)
            staticId = FName(TEXT("ClothArmor"));
            auto dynamicItemDatabase = DynamicItemHook.call<UPalDynamicItemDataBase*>(self, dynamicItemId, staticId, itemCreateParam);

            // This makes it so that when the game saves, this dynamic data will be excluded which permanently removes it from the save.
            dynamicItemDatabase->GetIgnoreOnSave() = true;
            return dynamicItemDatabase;
        }

        return DynamicItemHook.call<UPalDynamicItemDataBase*>(self, dynamicItemId, staticId, itemCreateParam);
    }
}