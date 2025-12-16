#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "SDK/Classes/PalStaticItemDataTable.h"
#include "SDK/Classes/PalStaticArmorItemData.h"
#include "SDK/Classes/PalStaticConsumeItemData.h"
#include "SDK/Classes/PalStaticWeaponItemData.h"
#include "SDK/Classes/PalDynamicWeaponItemDataBase.h"
#include "SDK/Classes/PalDynamicArmorItemDataBase.h"
#include "SDK/Structs/FPalCharacterIconDataRow.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Helpers/String.hpp"
#include "Utility/Logging.h"
#include "Loader/PalItemModLoader.h"
#include <SDK/Structs/FPalLocalizedTextData.h>

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalItemModLoader::PalItemModLoader() : PalModLoaderBase("items") {}

	PalItemModLoader::~PalItemModLoader() {}

	void PalItemModLoader::Initialize()
	{
		m_itemDataAsset = UECustom::UObjectGlobals::StaticFindObject<UPalStaticItemDataAsset*>(nullptr, nullptr,
			STR("/Game/Pal/DataAsset/Item/DA_StaticItemDataAsset.DA_StaticItemDataAsset"));

        m_itemRecipeTable = UECustom::UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
            STR("/Game/Pal/DataTable/Item/DT_ItemRecipeDataTable.DT_ItemRecipeDataTable"));

        m_nameTranslationTable = UECustom::UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
            STR("/Game/Pal/DataTable/Text/DT_ItemNameText.DT_ItemNameText"));

        m_descriptionTranslationTable = UECustom::UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
            STR("/Game/Pal/DataTable/Text/DT_ItemDescriptionText.DT_ItemDescriptionText"));

        PS::Log<LogLevel::Verbose>(STR("Initialized ItemModLoader\n"));

        InitializeDummyTranslations();
	}

	void PalItemModLoader::Load(const nlohmann::json& Data)
	{
		for (auto& [Key, Value] : Data.items())
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

    UPalStaticItemDataBase* PalItemModLoader::AddDummyItem(UPalStaticItemDataTable* StaticItemDataTable, const FName& ItemId)
    {
        auto DataAsset = StaticItemDataTable->GetDataAsset();

        UClass* DatabaseClass = UPalStaticItemDataBase::StaticClass();

        if (!DatabaseClass->GetPropertyByNameInChain(STR("ID")))
        {
            throw std::runtime_error("Property 'ID' has changed in DA_StaticItemDataAsset. Update to Pal Schema is needed.");
        }

        if (!DatabaseClass->GetPropertyByNameInChain(STR("DynamicItemDataClass")))
        {
            throw std::runtime_error("Property 'DynamicItemDataClass' has changed in DA_StaticItemDataAsset. Update to Pal Schema is needed.");
        }

        FStaticConstructObjectParameters ConstructParams(DatabaseClass, DataAsset);
        ConstructParams.Name = NAME_None;

        auto Item = UObjectGlobals::StaticConstructObject<UPalStaticItemDataBase*>(ConstructParams);

        auto IDProperty = Item->GetValuePtrByPropertyNameInChain<FName>(STR("ID"));
        if (IDProperty)
        {
            *IDProperty = ItemId;
        }

        auto OverrideNameMsgID = Item->GetValuePtrByPropertyNameInChain<FName>(STR("OverrideNameMsgID"));
        if (OverrideNameMsgID)
        {
            *OverrideNameMsgID = ItemId;
        }

        auto OverrideDescMsgID = Item->GetValuePtrByPropertyNameInChain<FName>(STR("OverrideDescMsgID"));
        if (OverrideDescMsgID)
        {
            *OverrideDescMsgID = FName(STR("ITEM_DESC_StaticDummy"), FNAME_Add);
        }

        auto IconTexture = Item->GetValuePtrByPropertyNameInChain<UECustom::TSoftObjectPtr<UObject>>(STR("IconTexture"));
        if (IconTexture)
        {
            *IconTexture = UECustom::TSoftObjectPtr<UObject>(UECustom::FSoftObjectPath(STR("/Game/Pal/Texture/UI/Main_Menu/T_icon_unknown.T_icon_unknown")));
        }

        auto TypeA = Item->GetValuePtrByPropertyNameInChain<uint8>(STR("TypeA"));
        if (TypeA)
        {
            *TypeA = 5U; // Material
        }

        auto TypeB = Item->GetValuePtrByPropertyNameInChain<uint8>(STR("TypeB"));
        if (TypeB)
        {
            *TypeB = 28U; // MaterialProcessing
        }

        auto SortID = Item->GetValuePtrByPropertyNameInChain<int>(STR("SortID"));
        if (SortID)
        {
            *SortID = 5001;
        }

        auto MaxStackCount = Item->GetValuePtrByPropertyNameInChain<int>(STR("MaxStackCount"));
        if (MaxStackCount)
        {
            *MaxStackCount = 9999;
        }

        DataAsset->StaticItemDataMap.Add(ItemId, Item);

        PS::Log<LogLevel::Verbose>(STR("Created a dummy item for {}\n"), ItemId.ToString());

        return Item;
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

    void PalItemModLoader::InitializeDummyTranslations()
    {
        FPalLocalizedTextData NewDescRow{};
        NewDescRow.TextData = FText(STR("A memory of a scrapped item from the past."));
        m_descriptionTranslationTable->AddRow(FName(STR("ITEM_DESC_StaticDummy"), FNAME_Add), NewDescRow);

        PS::Log<LogLevel::Verbose>(STR("Initialized Dummy Translations for ItemModLoader\n"));
    }
}