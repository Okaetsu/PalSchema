#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/FProperty.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/World.hpp"
#include "Unreal/GameplayStatics.hpp"
#include "SDK/Classes/KismetInternationalizationLibrary.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Utility/JsonHelpers.h"
#include "Helpers/String.hpp"
#include "SDK/Structs/Custom/FManagedStruct.h"
#include "SDK/Structs/FPalCharacterIconDataRow.h"
#include "SDK/Structs/FPalBPClassDataRow.h"
#include "SDK/Structs/FPalNPCTalkFlowClassDataRow.h"
#include "SDK/Structs/FPalItemShopLotteryDataRow.h"
#include "SDK/Structs/FPalItemShopSettingDataRow.h"
#include "Loader/PalHumanModLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalHumanModLoader::PalHumanModLoader() : PalModLoaderBase("npcs") {}

	PalHumanModLoader::~PalHumanModLoader() {}

    void PalHumanModLoader::OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase != EEngineLifecyclePhase::GameInstanceInit)
        {
            return;
        }

        PS::JsonHelpers::ParseJsonFilesInPath(loaderPath, [&](const nlohmann::json& data) {
            LoadHumans(data);
        });
    }

    bool PalHumanModLoader::CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase == EEngineLifecyclePhase::GameInstanceInit)
        {
            return true;
        }

        return false;
    }

    bool PalHumanModLoader::OnInitialize()
    {
        try
        {
            m_humanDataTable = GetDatatableByName("DT_PalHumanParameter");
            m_iconDataTable = GetDatatableByName("DT_PalCharacterIconDataTable");
            m_palBpClassTable = GetDatatableByName("DT_PalBPClass");
            m_dropItemTable = GetDatatableByName("DT_PalDropItem");
            m_npcNameTable = GetDatatableByName("DT_HumanNameText");
            m_palShortDescTable = GetDatatableByName("DT_PalShortDescriptionText");
            m_palLongDescTable = GetDatatableByName("DT_PalLongDescriptionText");
            m_npcTalkFlowTable = GetDatatableByName("DT_NPCTalkFlow");
            m_itemShopLotteryDataTable = GetDatatableByName("DT_ItemShopLotteryData");
            m_itemShopCreateDataTable = GetDatatableByName("DT_ItemShopCreateData");
            m_itemShopSettingDataTable = GetDatatableByName("DT_ItemShopSettingData");
        }
        catch (const std::exception& e)
        {
            PS::Log<LogLevel::Error>(STR("Unable to initialize {}, {}\n"), GetDisplayName(), RC::to_generic_string(e.what()));
            return false;
        }

        return true;
    }

    void PalHumanModLoader::LoadHumans(const nlohmann::json& data)
    {
        for (auto& [character_id, properties] : data.items())
        {
            auto CharacterId = FName(RC::to_generic_string(character_id), FNAME_Add);
            auto TableRow = m_humanDataTable->FindRowUnchecked(CharacterId);
            if (TableRow)
            {
                Edit(TableRow, CharacterId, properties);
            }
            else
            {
                Add(CharacterId, properties);
            }
        }
    }

	//Add New NPC
	void PalHumanModLoader::Add(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
	{
		auto NpcRowStruct = m_humanDataTable->GetRowStruct().Get();
        FManagedStruct NpcRowData{ NpcRowStruct };

        for (auto& [key, value] : properties.items())
        {
            if (key == "IconAssetPath")
            {
                auto IconPath = RC::to_generic_string(value.get<std::string>());
                AddIcon(CharacterId, IconPath);
            }
            else if (key == "BlueprintAssetPath")
            {
                auto BlueprintPath = RC::to_generic_string(value.get<std::string>());
                AddBlueprint(CharacterId, BlueprintPath);
            }
            else if (key == "Loot")
            {
                AddLoot(CharacterId, value);
            }
            else if (key == "Shop")
            {
                AddShop(CharacterId, value);
            }
            else
            {
                auto KeyName = RC::to_generic_string(key);
                auto Property = NpcRowStruct->GetPropertyByName(KeyName.c_str());
                if (Property)
                {
                    PropertyHelper::CopyJsonValueToContainer(NpcRowData.GetData(), Property, value);
                }
            }
        }

        m_humanDataTable->AddRow(CharacterId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(NpcRowData.GetData()));

        AddTranslations(CharacterId, properties);

        PS::Log<RC::LogLevel::Normal>(STR("Added new npc '{}'\n"), CharacterId.ToString());
    }


    //Edit Existing NPC
    void PalHumanModLoader::Edit(uint8_t* TableRow, const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
    {
        PS::Log<RC::LogLevel::Normal>(STR("Called Edit NPC for {}"), CharacterId.ToString());

        auto RowStruct = m_humanDataTable->GetRowStruct().Get();
		for (auto& [key, value] : properties.items())
		{
			if (key == "IconAssetPath")
			{
				auto IconTableRow = std::bit_cast<FPalCharacterIconDataRow*>(m_iconDataTable->FindRowUnchecked(CharacterId));
				if (IconTableRow)
				{
					auto IconPath = RC::to_generic_string(value.get<std::string>());
					IconTableRow->Icon = UECustom::TSoftObjectPtr<UECustom::UTexture2D>(UECustom::FSoftObjectPath(IconPath));
				}
			}
			else if (key == "BlueprintAssetPath")
			{
				auto BlueprintTableRow = std::bit_cast<FPalBPClassDataRow*>(m_palBpClassTable->FindRowUnchecked(CharacterId));
				if (BlueprintTableRow)
				{
					auto BlueprintPath = RC::to_generic_string(value.get<std::string>());
					BlueprintTableRow->BPClass = UECustom::TSoftClassPtr<RC::Unreal::UClass>(UECustom::FSoftObjectPath(BlueprintPath));
				}
			}
            else if (key == "Loot")
            {
				AddLoot(CharacterId, value);
            }
			else
			{
                auto KeyName = RC::to_generic_string(key);
				auto Property = RowStruct->GetPropertyByName(KeyName.c_str());
				if (Property)
				{
					PropertyHelper::CopyJsonValueToContainer(TableRow, Property, value);
				}
			}
		}

		EditTranslations(CharacterId, properties);

	}

	//Add BP
	void PalHumanModLoader::AddBlueprint(const RC::Unreal::FName& CharacterId, const RC::StringType& BlueprintPath)
	{
		FPalBPClassDataRow BlueprintDataRow{ BlueprintPath };
		m_palBpClassTable->AddRow(CharacterId, BlueprintDataRow);
	}

	//Add Icon
	void PalHumanModLoader::AddIcon(const RC::Unreal::FName& CharacterId, const RC::StringType& IconPath)
	{
		FPalCharacterIconDataRow IconDataRow{ IconPath };
		m_iconDataTable->AddRow(CharacterId, IconDataRow);
	}

	//Add Loot Drops
	void PalHumanModLoader::AddLoot(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
	{
		auto RowStruct = m_dropItemTable->GetRowStruct().Get();
        FManagedStruct NpcDropItemData{ RowStruct };

		auto CharacterIdProperty = RowStruct->GetPropertyByName(STR("CharacterId"));
		if (!CharacterIdProperty)
		{
			throw std::runtime_error("Property CharacterId doesn't exist in DT_PalDropItem, which means you should wait for an update as something in the table has changed.");
		}

		FMemory::Memcpy(CharacterIdProperty->ContainerPtrToValuePtr<void>(NpcDropItemData.GetData()), &CharacterId, sizeof(FName));

		auto Index = 1;
		auto loot_array = properties.get<std::vector<nlohmann::json>>();
		for (auto& loot : loot_array)
		{
			if (!loot.contains("ItemId"))
			{
				PS::Log<RC::LogLevel::Error>(STR("ItemId was not specified in {}, skipping loot entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!loot.contains("DropChance"))
			{
				PS::Log<RC::LogLevel::Error>(STR("DropChance was not specified in {}, skipping loot entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!loot.contains("Min"))
			{
				PS::Log<RC::LogLevel::Error>(STR("Min was not specified in {}, skipping loot entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!loot.contains("Max"))
			{
				PS::Log<RC::LogLevel::Error>(STR("Max was not specified in {}, skipping loot entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!loot.at("ItemId").is_string())
			{
				PS::Log<RC::LogLevel::Error>(STR("ItemId in {} must be a string, skipping loot entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!loot.at("DropChance").is_number_float())
			{
				PS::Log<RC::LogLevel::Error>(STR("DropChance in {} must be a float, skipping loot entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!loot.at("Min").is_number_integer())
			{
				PS::Log<RC::LogLevel::Error>(STR("Min in {} must be an integer, skipping loot entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!loot.at("Max").is_number_integer())
			{
				PS::Log<RC::LogLevel::Error>(STR("Max in {} must be an integer, skipping loot entry.\n"), CharacterId.ToString());
				continue;
			}

			auto ItemIdWithSuffix = std::format(STR("ItemId{}"), Index);
			auto ItemIdProperty = RowStruct->GetPropertyByName(ItemIdWithSuffix.c_str());
			if (!ItemIdProperty)
			{
				throw std::runtime_error(std::format("Property 'ItemId{}' doesn't exist in DT_PalDropItem, Pal Schema needs an update.", Index));
			}

			auto RateWithSuffix = std::format(STR("Rate{}"), Index);
			auto RateProperty = RowStruct->GetPropertyByName(RateWithSuffix.c_str());
			if (!RateProperty)
			{
				throw std::runtime_error(std::format("Property 'Rate{}' doesn't exist in DT_PalDropItem, Pal Schema needs an update.", Index));
			}

			auto MaxWithSuffix = std::format(STR("Max{}"), Index);
			auto MaxProperty = RowStruct->GetPropertyByName(MaxWithSuffix.c_str());
			if (!MaxProperty)
			{
				throw std::runtime_error(std::format("Property 'Max{}' doesn't exist in DT_PalDropItem, Pal Schema needs an update.", Index));
			}

			auto MinWithSuffix = std::format(STR("min{}"), Index);
			auto MinProperty = RowStruct->GetPropertyByName(MinWithSuffix.c_str());
			if (!MinProperty)
			{
				throw std::runtime_error(std::format("Property 'min{}' doesn't exist in DT_PalDropItem, Pal Schema needs an update.", Index));
			}

			auto& ItemId = loot.at("ItemId");
			auto& DropChance = loot.at("DropChance");
			auto& Min = loot.at("Min");
			auto& Max = loot.at("Max");

			PropertyHelper::CopyJsonValueToContainer(NpcDropItemData.GetData(), ItemIdProperty, ItemId);
			PropertyHelper::CopyJsonValueToContainer(NpcDropItemData.GetData(), RateProperty, DropChance);
			PropertyHelper::CopyJsonValueToContainer(NpcDropItemData.GetData(), MinProperty, Min);
			PropertyHelper::CopyJsonValueToContainer(NpcDropItemData.GetData(), MaxProperty, Max);

			Index++;

			if (Index > 10)
			{
				break;
			}
		}

		auto RowName = std::format(STR("{}000"), CharacterId.ToString());
		m_dropItemTable->AddRow(FName(RowName, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(NpcDropItemData.GetData()));
	}

	//Add New NPC Translations
	void PalHumanModLoader::AddTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data)
	{
		if (Data.contains("Name"))
		{
			auto FixedCharacterId = std::format(STR("HUMAN_NAME_{}"), CharacterId.ToString());
			auto TranslationRowStruct = m_npcNameTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
                FManagedStruct TranslationRowData{ TranslationRowStruct };
				try
				{
					PropertyHelper::CopyJsonValueToContainer(TranslationRowData.GetData(), TextProperty, Data.at("Name"));
				}
				catch (const std::exception& e)
				{
					throw std::runtime_error(e.what());
				}

				m_npcNameTable->AddRow(FName(FixedCharacterId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(TranslationRowData.GetData()));
			}
		}

		// Long & Short descriptions seem to be using default human description.
		
	}

	//Edit Translations
	void PalHumanModLoader::EditTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data)
	{
		if (Data.contains("Name"))
		{
			auto FixedCharacterId = std::format(STR("HUMAN_NAME_{}"), CharacterId.ToString());
			auto TranslationRowStruct = m_npcNameTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = m_npcNameTable->FindRowUnchecked(FName(FixedCharacterId, FNAME_Add));
				if (Row)
				{
					PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data.at("Name"));
				}
			}
		}

		// Long & Short descriptions seem to be using default human description.

	}

	void PalHumanModLoader::AddShop(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data)
	{
		if (!Data.contains("CanSell")) {
			PS::Log<RC::LogLevel::Error>(STR("CanSell was not specified in {}, skipping shop entry.\n"), CharacterId.ToString());
			return;
		}
		if (!Data.contains("ShopTableId")) {
			PS::Log<RC::LogLevel::Error>(STR("ShopTableId was not specified in {}, skipping shop entry.\n"), CharacterId.ToString());
			return;
		}
		if (!Data.contains("Currency")) {
			PS::Log<RC::LogLevel::Error>(STR("Currency was not specified in {}, skipping shop entry.\n"), CharacterId.ToString());
			return;
		}
		if (!Data.contains("Items")) {
			PS::Log<RC::LogLevel::Error>(STR("Items was not specified in {}, skipping shop entry.\n"), CharacterId.ToString());
			return;
		}
		if (!Data.at("CanSell").is_boolean()) {
			PS::Log<RC::LogLevel::Error>(STR("CanSell in {} must be a boolean, skipping shop entry.\n"), CharacterId.ToString());
			return;
		}
		if (!Data.at("ShopTableId").is_string()) {
			PS::Log<RC::LogLevel::Error>(STR("ShopTableId in {} must be a string, skipping shop entry.\n"), CharacterId.ToString());
			return;
		}
		if (!Data.at("Currency").is_string()) {
			PS::Log<RC::LogLevel::Error>(STR("Currency in {} must be a string, skipping shop entry.\n"), CharacterId.ToString());
			return;
		}
		if (!Data.at("Items").is_array()) {
			PS::Log<RC::LogLevel::Error>(STR("Items in {} must be an array, skipping shop entry.\n"), CharacterId.ToString());
			return;
		}

		bool addSucceeded = true;

		if (Data.at("CanSell").get<bool>()) {
			FPalNPCTalkFlowClassDataRow NpcTalkFlowClassDataRow {STR("/Game/Pal/Blueprint/FlowGraph/NPCTalkFlow/Graph/FABP_CommonItemShop.FABP_CommonItemShop")};
			if (m_npcTalkFlowTable)
			{
				m_npcTalkFlowTable->AddRow(CharacterId, NpcTalkFlowClassDataRow);
				if (!m_npcTalkFlowTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
			}
			else
			{
				PS::Log<RC::LogLevel::Warning>(STR("npcTalkFlowTable not found, skipping adding talk flow for {}\n"), CharacterId.ToString());
				addSucceeded = false;
			}
		} else {
			FPalNPCTalkFlowClassDataRow NpcTalkFlowClassDataRow {STR("/Game/Pal/Blueprint/FlowGraph/NPCTalkFlow/Graph/FABP_CommonItemShop_WithoutSell.FABP_CommonItemShop_WithoutSell")};
			if (m_npcTalkFlowTable)
			{
				m_npcTalkFlowTable->AddRow(CharacterId, NpcTalkFlowClassDataRow);
				if (!m_npcTalkFlowTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
			}
			else
			{
				PS::Log<RC::LogLevel::Warning>(STR("npcTalkFlowTable not found, skipping adding talk flow for {}\n"), CharacterId.ToString());
				addSucceeded = false;
			}
		}

		auto ShopTableId = RC::to_generic_string(Data.at("ShopTableId").get<std::string>());
		if (m_itemShopLotteryDataTable)
		{
			FPalItemShopLotteryEntry entry{};
			entry.ShopGroupName = CharacterId;
			entry.Weight = 100;

			FPalItemShopLotteryDataRow row{};
			row.lotteryDataArray.Add(entry);

			m_itemShopLotteryDataTable->AddRow(FName(ShopTableId, FNAME_Add), row);
			if (!m_itemShopLotteryDataTable->FindRowUnchecked(FName(ShopTableId, FNAME_Add))) addSucceeded = false;
		}
		else
		{
			PS::Log<RC::LogLevel::Warning>(STR("ItemShopLotteryDataTable not found, skipping adding lottery row for {}\n"), CharacterId.ToString());
			addSucceeded = false;
		}
		if (m_itemShopCreateDataTable)
		{
			auto CreateRowJson = nlohmann::json::object();
			CreateRowJson["productDataArray"] = Data.at("Items");

			auto ExistingCreateRow = m_itemShopCreateDataTable->FindRowUnchecked(CharacterId);
			auto CreateRowStruct = m_itemShopCreateDataTable->GetRowStruct().Get();
			if (ExistingCreateRow)
			{
				try
				{
                    for (FProperty* Property : TFieldRange<FProperty>(CreateRowStruct, EFieldIterationFlags::IncludeSuper))
                    {
                        auto PropertyName = RC::to_string(Property->GetName());
                        if (CreateRowJson.contains(PropertyName))
                        {
                            PropertyHelper::CopyJsonValueToContainer(ExistingCreateRow, Property, CreateRowJson.at(PropertyName));
                        }
                    }
					if (!m_itemShopCreateDataTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
				}
				catch (const std::exception& e)
				{
					PS::Log<RC::LogLevel::Error>(STR("Failed to modify Row '{}' in {}: {}\n"), CharacterId.ToString(), m_itemShopCreateDataTable->GetFullName(), RC::to_generic_string(e.what()));
					addSucceeded = false;
				}
			}
			else
			{
				FManagedStruct CreateRowData{ CreateRowStruct };
				try
				{
                    for (FProperty* Property : TFieldRange<FProperty>(CreateRowStruct, EFieldIterationFlags::IncludeSuper))
                    {
                        auto PropertyName = RC::to_string(Property->GetName());
                        if (CreateRowJson.contains(PropertyName))
                        {
                            PropertyHelper::CopyJsonValueToContainer(CreateRowData.GetData(), Property, CreateRowJson.at(PropertyName));
                        }
                    }
					m_itemShopCreateDataTable->AddRow(CharacterId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(CreateRowData.GetData()));
					if (!m_itemShopCreateDataTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
				}
				catch (const std::exception& e)
				{
					PS::Log<RC::LogLevel::Error>(STR("Failed to add Row '{}' to {}: {}\n"), CharacterId.ToString(), m_itemShopCreateDataTable->GetFullName(), RC::to_generic_string(e.what()));
					addSucceeded = false;
				}
			}
		}
		else
		{
			PS::Log<RC::LogLevel::Warning>(STR("ItemShopCreateDataTable not found, skipping adding create-data row for {}\n"), CharacterId.ToString());
			addSucceeded = false;
		}
		if (m_itemShopSettingDataTable)
		{
			auto CurrencyStr = RC::to_generic_string(Data.at("Currency").get<std::string>());
			FPalItemShopSettingDataRow settingRow{ CurrencyStr };

			auto ExistingRow = m_itemShopSettingDataTable->FindRowUnchecked(CharacterId);
			if (ExistingRow)
			{
				auto RowStruct = m_itemShopSettingDataTable->GetRowStruct().Get();
				auto CurrencyProp = RowStruct->GetPropertyByName(STR("CurrencyItemID"));
				if (CurrencyProp)
				{
					PropertyHelper::CopyJsonValueToContainer(ExistingRow, CurrencyProp, CurrencyStr);
				}
				else
				{
					PS::Log<RC::LogLevel::Warning>(STR("CurrencyItemID property not found in ItemShopSettingData row struct, skipping update for {}\n"), CharacterId.ToString());
				}
				if (!m_itemShopSettingDataTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
			}
			else
			{
				m_itemShopSettingDataTable->AddRow(CharacterId, settingRow);
				if (!m_itemShopSettingDataTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
			}
		}
		else
		{
			PS::Log<RC::LogLevel::Warning>(STR("ItemShopSettingDataTable not found, skipping adding setting row for {}\n"), CharacterId.ToString());
			addSucceeded = false;
		}

		if (addSucceeded)
		{
			PS::Log<RC::LogLevel::Normal>(STR("Added shop for {} (ShopTableId: {})\n"), CharacterId.ToString(), ShopTableId);
		} else {
			PS::Log<RC::LogLevel::Error>(STR("Failed to fully add shop for {} (ShopTableId: {}) - some parts were not added correctly.\n"), CharacterId.ToString(), ShopTableId);
		}
	}
}