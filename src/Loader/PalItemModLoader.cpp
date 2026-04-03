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
                TEXT("/Game/Pal/DataAsset/Item/DA_StaticItemDataAsset.DA_StaticItemDataAsset"));

            m_itemDataTable = GetDatatableByName("DT_ItemDataTable");
            m_itemRecipeTable = GetDatatableByName("DT_ItemRecipeDataTable");
            m_nameTranslationTable = GetDatatableByName("DT_ItemNameText");
            m_descriptionTranslationTable = GetDatatableByName("DT_ItemDescriptionText");

            SetupHooks();
        }
        catch (const std::exception& e)
        {
            PS::Log<LogLevel::Error>(TEXT("Unable to initialize {}, {}\n"), GetDisplayName(), RC::to_generic_string(e.what()));
            return false;
        }

        return true;
    }

    void PalItemModLoader::LoadItems(const nlohmann::json& data)
    {
        for (auto& [key, value] : data.items())
        {
            auto itemId = FName(RC::to_generic_string(key), FNAME_Add);
            auto row = m_itemDataAsset->StaticItemDataMap.Find(itemId);
            if (value.is_null())
            {
                if (!row) return;
                m_itemDataAsset->StaticItemDataMap.Remove(itemId);
                PS::Log<RC::LogLevel::Normal>(TEXT("Deleted Item '{}'\n"), itemId.ToString());
            }
            else
            {
                if (row)
                {
                    Edit(itemId, *row, value);
                    PS::Log<RC::LogLevel::Normal>(TEXT("Modified Item '{}'\n"), itemId.ToString());
                }
                else
                {
                    Add(itemId, value);
                    PS::Log<RC::LogLevel::Normal>(TEXT("Added Item '{}'\n"), itemId.ToString());
                }
            }
        }
    }

	void PalItemModLoader::Add(const RC::Unreal::FName& itemId, const nlohmann::json& data)
	{
		if (itemId == NAME_None)
		{
			throw std::runtime_error("ID was set to None");
		}

		if (!data.contains("Type"))
		{
			throw std::runtime_error(std::format("You must supply a Type field in {} when adding new items", RC::to_string(itemId.ToString())));
		}

		if (!data.at("Type").is_string())
		{
			throw std::runtime_error(std::format("Type must be a string in {}", RC::to_string(itemId.ToString())));
		}

		auto type = data.at("Type").get<std::string>();

		UClass* databaseClass = nullptr;
		UClass* dynamicDatabaseClass = nullptr;
		if (type == "Armor" || type == "PalStaticArmorItemData")
		{
			databaseClass = UPalStaticArmorItemData::StaticClass();
			dynamicDatabaseClass = UPalDynamicArmorItemDataBase::StaticClass();
		}
		else if (type == "Weapon" || type == "PalStaticWeaponItemData")
		{
			databaseClass = UPalStaticWeaponItemData::StaticClass();
			dynamicDatabaseClass = UPalDynamicWeaponItemDataBase::StaticClass();
		}
		else if (type == "Consumable" || type == "PalStaticConsumeItemData")
		{
			databaseClass = UPalStaticConsumeItemData::StaticClass();
		}
		else if (type == "Generic" || type == "PalStaticItemDataBase")
		{
			databaseClass = UPalStaticItemDataBase::StaticClass();
		}
		else
		{
			throw std::runtime_error(std::format("Type {} in {} isn't supported, must be Armor, Weapon, Consumable or Generic", type, RC::to_string(itemId.ToString())));
		}

		if (!databaseClass->GetPropertyByNameInChain(TEXT("ID")))
		{
			throw std::runtime_error("Property 'ID' has changed in DA_StaticItemDataAsset. Update to Pal Schema is needed.");
		}

		if (!databaseClass->GetPropertyByNameInChain(TEXT("DynamicItemDataClass")))
		{
			throw std::runtime_error("Property 'DynamicItemDataClass' has changed in DA_StaticItemDataAsset. Update to Pal Schema is needed.");
		}

		FStaticConstructObjectParameters constructParams(databaseClass, m_itemDataAsset);
		constructParams.Name = NAME_None;

		auto item = UObjectGlobals::StaticConstructObject<UPalStaticItemDataBase*>(constructParams);

		auto idProperty = item->GetValuePtrByPropertyNameInChain<FName>(TEXT("ID"));
		if (idProperty)
		{
			*idProperty = itemId;
		}

		auto dynamicItemDataClassProperty = item->GetValuePtrByPropertyNameInChain<UClass*>(TEXT("DynamicItemDataClass"));
		if (dynamicItemDataClassProperty)
		{
			*dynamicItemDataClassProperty = dynamicDatabaseClass;
		}

        for (FProperty* property : TFieldRange<FProperty>(databaseClass, EFieldIterationFlags::IncludeSuper))
        {
            auto propertyName = RC::to_string(property->GetName());
            if (propertyName == "DynamicItemDataClass")
            {
                // We've already set this earlier so we skip it.
                continue;
            }
            if (data.contains(propertyName))
            {
                PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(item), property, data.at(propertyName));
            }
        }
		
		if (data.contains("Recipe"))
		{
			AddRecipe(itemId, data.at("Recipe"));
		}

        AddItemData(itemId, data);

		AddTranslations(itemId, data);

		m_itemDataAsset->StaticItemDataMap.Add(itemId, item);
	}

	void PalItemModLoader::Edit(const RC::Unreal::FName& itemId, UPalStaticItemDataBase* item, const nlohmann::json& data)
	{
        for (FProperty* property : TFieldRange<FProperty>(item->GetClassPrivate(), EFieldIterationFlags::IncludeSuper))
        {
            auto propertyName = RC::to_string(property->GetName());
            if (propertyName == "DynamicItemDataClass")
            {
                continue;
            }
            if (propertyName == "ID")
            {
                // Editing the ID is a bad idea, hence we skip it.
                continue;
            }
            if (data.contains(propertyName))
            {
                PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(item), property, data.at(propertyName));
            }
        }

		if (data.contains("Recipe"))
		{
			EditRecipe(itemId, data.at("Recipe"));
		}

		EditTranslations(itemId, data);
	}

	void PalItemModLoader::AddRecipe(const RC::Unreal::FName& itemId, const nlohmann::json& recipe)
	{
		auto rowStruct = m_itemRecipeTable->GetRowStruct().Get();

		auto itemRecipeData = FMemory::Malloc(rowStruct->GetStructureSize());
		rowStruct->InitializeStruct(itemRecipeData);

		for (auto& [propertyName, propertyValue] : recipe.items())
		{
			auto propertyNameWide = RC::to_generic_string(propertyName);

			if (propertyNameWide == TEXT("Product_Id"))
            {
                // We will set this later based on the key used for the json object, so we skip it for now.
                continue;
            }

            if (propertyNameWide == TEXT("Editor_RowNameHash"))
			{
				// We don't need to change this due to it being editor related, skip.
				continue;
			}

			auto property = rowStruct->GetPropertyByName(propertyNameWide.c_str());
			if (property)
			{
				try
				{
					PropertyHelper::CopyJsonValueToContainer(itemRecipeData, property, propertyValue);
				}
				catch (const std::exception& e)
				{
					FMemory::Free(itemRecipeData);
					throw std::runtime_error(e.what());
				}
			}
		}

		auto productIdProperty = rowStruct->GetPropertyByName(TEXT("Product_Id"));
		if (productIdProperty)
		{
			FMemory::Memcpy(productIdProperty->ContainerPtrToValuePtr<void>(itemRecipeData), &itemId, sizeof(FName));
		}

		m_itemRecipeTable->AddRow(itemId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(itemRecipeData));

        PS::Log<LogLevel::Normal>(TEXT("Added new Recipe for Item '{}'.\n"), itemId.ToString());
	}

	void PalItemModLoader::EditRecipe(const RC::Unreal::FName& itemId, const nlohmann::json& recipe)
	{
		auto rowStruct = m_itemRecipeTable->GetRowStruct().Get();

		auto recipeRow = m_itemRecipeTable->FindRowUnchecked(itemId);
		if (!recipeRow)
		{
			throw std::runtime_error(std::format("Row for Recipe '{}' doesn't exist", RC::to_string(itemId.ToString())));
		}

		for (auto& [propertyName, propertyValue] : recipe.items())
		{
			auto propertyNameWide = RC::to_generic_string(propertyName);
            if (propertyNameWide == TEXT("Editor_RowNameHash"))
			{
				// We don't need to change this due to it being editor related, skip.
				continue;
			}

			auto property = rowStruct->GetPropertyByName(propertyNameWide.c_str());
			if (property)
			{
				PropertyHelper::CopyJsonValueToContainer(recipeRow, property, propertyValue);
			}
		}

        PS::Log<LogLevel::Normal>(TEXT("Modified Recipe for Item '{}'.\n"), itemId.ToString());
	}

	void PalItemModLoader::AddTranslations(const RC::Unreal::FName& itemId, const nlohmann::json& data)
	{
		if (data.contains("Name"))
		{
			auto rowId = std::format(TEXT("ITEM_NAME_{}"), itemId.ToString());
			auto rowStruct = m_nameTranslationTable->GetRowStruct().Get();
			auto textDataProperty = rowStruct->GetPropertyByName(TEXT("TextData"));
            if (textDataProperty)
            {
                auto rowData = FMemory::Malloc(rowStruct->GetStructureSize());
                rowStruct->InitializeStruct(rowData);

                try
                {
                    PropertyHelper::CopyJsonValueToContainer(rowData, textDataProperty, data.at("Name"));
                }
                catch (const std::exception& e)
                {
                    FMemory::Free(rowData);
					throw std::runtime_error(e.what());
				}

				m_nameTranslationTable->AddRow(FName(rowId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(rowData));
			}
		}

		if (data.contains("Description"))
		{
			auto rowId = std::format(TEXT("ITEM_DESC_{}"), itemId.ToString());
            auto rowStruct = m_descriptionTranslationTable->GetRowStruct().Get();
            auto textDataProperty = rowStruct->GetPropertyByName(TEXT("TextData"));
            if (textDataProperty)
            {
                auto rowData = FMemory::Malloc(rowStruct->GetStructureSize());
                rowStruct->InitializeStruct(rowData);

                try
                {
                    PropertyHelper::CopyJsonValueToContainer(rowData, textDataProperty, data.at("Description"));
                }
                catch (const std::exception& e)
                {
                    FMemory::Free(rowData);
                    throw std::runtime_error(e.what());
                }

                m_descriptionTranslationTable->AddRow(FName(rowId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(rowData));
			}
		}
	}

	void PalItemModLoader::EditTranslations(const RC::Unreal::FName& itemId, const nlohmann::json& data)
	{
		if (data.contains("Name"))
		{
			auto rowId = std::format(TEXT("ITEM_NAME_{}"), itemId.ToString());
			auto rowStruct = m_nameTranslationTable->GetRowStruct().Get();
			auto textDataProperty = rowStruct->GetPropertyByName(TEXT("TextData"));
			if (textDataProperty)
			{
				auto row = m_nameTranslationTable->FindRowUnchecked(FName(rowId, FNAME_Add));
				if (row)
				{
					PropertyHelper::CopyJsonValueToContainer(row, textDataProperty, data.at("Name"));
				}
			}
		}

		if (data.contains("Description"))
		{
			auto rowId = std::format(TEXT("ITEM_DESC_{}"), itemId.ToString());
			auto rowStruct = m_nameTranslationTable->GetRowStruct().Get();
			auto textDataProperty = rowStruct->GetPropertyByName(TEXT("TextData"));
			if (textDataProperty)
			{
				auto row = m_descriptionTranslationTable->FindRowUnchecked(FName(rowId, FNAME_Add));
				if (row)
				{
					PropertyHelper::CopyJsonValueToContainer(row, textDataProperty, data.at("Description"));
				}
			}
		}
	}

    void PalItemModLoader::AddItemData(const RC::Unreal::FName& itemId, const nlohmann::json& data)
    {
        auto rowStruct = m_itemDataTable->GetRowStruct().Get();

        auto legalProp = rowStruct->GetPropertyByName(TEXT("bLegalInGame"));
        if (!legalProp)
        {
            PS::Log<LogLevel::Error>(TEXT("Property 'bLegalInGame' does not exist in DT_ItemDataTable, skipping addition of data for {}.\n"), itemId.ToString());
            return;
        }

        FManagedStruct rowData{ rowStruct };

        if (data.contains("bLegalInGame"))
        {
            PropertyHelper::CopyJsonValueToContainer(rowData.GetData(), legalProp, data.at("bLegalInGame"));
        }
        else
        {
            bool legalValue = true;
            FMemory::Memcpy(legalProp->ContainerPtrToValuePtr<void>(rowData.GetData()), &legalValue, sizeof(bool));
        }

        m_itemDataTable->AddRow(itemId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(rowData.GetData()));
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
            PS::Log<LogLevel::Error>(TEXT("{}. PalSchema will be unable to clean up invalid items and worlds with invalid items will crash on load.\n"), 
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
            PS::Log<LogLevel::Warning>(TEXT("Item '{}' is invalid. Deleting.\n"), itemId->StaticId.ToString());
            itemId->StaticId = NAME_None;
            amount = 0;
        }

        UpdateItem_ServerInternalHook.call(self, itemId, amount, param4, param5);
    }

    UPalDynamicItemDataBase* PalItemModLoader::CreateDynamicItemDatabase_Detour(RC::Unreal::UObject* self, FPalDynamicItemId* dynamicItemId, RC::Unreal::FName staticId, void* itemCreateParam)
    {
        if (_ReturnAddress() == ApplyDynamicItemSaveDataAddress && !IsValidItem(self, staticId))
        {
            PS::Log<LogLevel::Warning>(TEXT("Item '{}' is invalid. Dynamic data for this item will be deleted on next save.\n"), staticId.ToString());

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