#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "SDK/Classes/UDataTable.h"
#include "SDK/Classes/KismetInternationalizationLibrary.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
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

	void PalHumanModLoader::Initialize()
	{
		n_dataTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Character/DT_PalHumanParameter.DT_PalHumanParameter"));

		n_iconDataTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr, 
			STR("/Game/Pal/DataTable/Character/DT_PalCharacterIconDataTable.DT_PalCharacterIconDataTable"));

		n_palBpClassTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Character/DT_PalBPClass.DT_PalBPClass"));

		n_dropItemTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Character/DT_PalDropItem.DT_PalDropItem"));

		n_npcNameTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_HumanNameText.DT_HumanNameText"));

		n_palShortDescTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_PalShortDescriptionText.DT_PalShortDescriptionText"));

		n_palLongDescTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_PalLongDescriptionText.DT_PalLongDescriptionText"));

		n_npcTalkFlowTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/Blueprint/Component/NPCTalk/DT_NPCTalkFlow.DT_NPCTalkFlow"));

		n_ItemShopLotteryDataTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/ItemShop/DT_ItemShopLotteryData.DT_ItemShopLotteryData"));

		n_ItemShopCreateDataTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/ItemShop/DT_ItemShopCreateData.DT_ItemShopCreateData"));

		n_ItemShopSettingDataTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/ItemShop/DT_ItemShopSettingData.DT_ItemShopSettingData"));
	}

	void PalHumanModLoader::Load(const nlohmann::json& json)
	{
		for (auto& [character_id, properties] : json.items())
		{
			auto CharacterId = FName(RC::to_generic_string(character_id), FNAME_Add);
			auto TableRow = n_dataTable->FindRowUnchecked(CharacterId);
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
		auto NpcRowStruct = n_dataTable->GetRowStruct().Get();
		auto NpcRowData = FMemory::Malloc(NpcRowStruct->GetStructureSize());
		NpcRowStruct->InitializeStruct(NpcRowData);

		for (auto& [key, value] : properties.items())
		{
			auto KeyName = RC::to_generic_string(key);
			if (KeyName == STR("IconAssetPath"))
			{
				auto IconPath = RC::to_generic_string(value.get<std::string>());
				AddIcon(CharacterId, IconPath);
			}
			else if (KeyName == STR("BlueprintAssetPath"))
			{
				auto BlueprintPath = RC::to_generic_string(value.get<std::string>());
				AddBlueprint(CharacterId, BlueprintPath);
			}
			else if (KeyName == STR("Loot"))
			{
				AddLoot(CharacterId, value);
			}
			else if (KeyName == STR("Shop"))
			{
				AddShop(CharacterId, value);
			}
			else
			{
				auto Property = NpcRowStruct->GetPropertyByName(KeyName.c_str());
				if (Property)
				{
					PropertyHelper::CopyJsonValueToContainer(NpcRowData, Property, value);
				}
			}
		}

		n_dataTable->AddRow(CharacterId, *reinterpret_cast<UECustom::FTableRowBase*>(NpcRowData));

		AddTranslations(CharacterId, properties);

		PS::Log<RC::LogLevel::Normal>(STR("Added new npc '{}'\n"), CharacterId.ToString());
	}


	//Edit Existing NPC
	void PalHumanModLoader::Edit(uint8_t* TableRow, const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
	{
		PS::Log<RC::LogLevel::Normal>(STR("Called Edit NPC for {}"), CharacterId.ToString());

		auto RowStruct = n_dataTable->GetRowStruct().Get();
		for (auto& [key, value] : properties.items())
		{
			auto KeyName = RC::to_generic_string(key);
			if (KeyName == STR("IconAssetPath"))
			{
				auto IconTableRow = std::bit_cast<FPalCharacterIconDataRow*>(n_iconDataTable->FindRowUnchecked(CharacterId));
				if (IconTableRow)
				{
					auto IconPath = RC::to_generic_string(value.get<std::string>());
					IconTableRow->Icon = UECustom::TSoftObjectPtr<UECustom::UTexture2D>(UECustom::FSoftObjectPath(IconPath));
				}
			}
			else if (KeyName == STR("BlueprintAssetPath"))
			{
				auto BlueprintTableRow = std::bit_cast<FPalBPClassDataRow*>(n_palBpClassTable->FindRowUnchecked(CharacterId));
				if (BlueprintTableRow)
				{
					auto BlueprintPath = RC::to_generic_string(value.get<std::string>());
					BlueprintTableRow->BPClass = UECustom::TSoftClassPtr<RC::Unreal::UClass>(UECustom::FSoftObjectPath(BlueprintPath));
				}
			}
            else if (KeyName == STR("Loot"))
            {
				AddLoot(CharacterId, value);
            }
			else
			{
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
		n_palBpClassTable->AddRow(CharacterId, BlueprintDataRow);
	}

	//Add Icon
	void PalHumanModLoader::AddIcon(const RC::Unreal::FName& CharacterId, const RC::StringType& IconPath)
	{
		FPalCharacterIconDataRow IconDataRow{ IconPath };
		n_iconDataTable->AddRow(CharacterId, IconDataRow);
	}

	//Add Loot Drops
	void PalHumanModLoader::AddLoot(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
	{
		auto RowStruct = n_dropItemTable->GetRowStruct().Get();

		auto NpcDropItemData = FMemory::Malloc(RowStruct->GetStructureSize());
		RowStruct->InitializeStruct(NpcDropItemData);

		auto CharacterIdProperty = RowStruct->GetPropertyByName(STR("CharacterId"));
		if (!CharacterIdProperty)
		{
			FMemory::Free(NpcDropItemData);
			throw std::runtime_error("Property CharacterId doesn't exist in DT_PalDropItem, which means you should wait for an update as something in the table has changed.");
		}

		FMemory::Memcpy(CharacterIdProperty->ContainerPtrToValuePtr<void>(NpcDropItemData), &CharacterId, sizeof(FName));

		auto Index = 1;
		auto loot_array = properties.get<std::vector<nlohmann::json>>();
		for (auto& loot : loot_array)
		{
			auto IndexString = std::to_wstring(Index);

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

			auto ItemIdWithSuffix = std::format(STR("ItemId{}"), IndexString);
			auto ItemIdProperty = RowStruct->GetPropertyByName(ItemIdWithSuffix.c_str());
			if (!ItemIdProperty)
			{
				throw std::runtime_error(std::format("Property 'ItemId{}' doesn't exist in DT_PalDropItem, Pal Schema needs an update.", Index));
			}

			auto RateWithSuffix = std::format(STR("Rate{}"), IndexString);
			auto RateProperty = RowStruct->GetPropertyByName(RateWithSuffix.c_str());
			if (!RateProperty)
			{
				throw std::runtime_error(std::format("Property 'Rate{}' doesn't exist in DT_PalDropItem, Pal Schema needs an update.", Index));
			}

			auto MaxWithSuffix = std::format(STR("Max{}"), IndexString);
			auto MaxProperty = RowStruct->GetPropertyByName(MaxWithSuffix.c_str());
			if (!MaxProperty)
			{
				throw std::runtime_error(std::format("Property 'Max{}' doesn't exist in DT_PalDropItem, Pal Schema needs an update.", Index));
			}

			auto MinWithSuffix = std::format(STR("min{}"), IndexString);
			auto MinProperty = RowStruct->GetPropertyByName(MinWithSuffix.c_str());
			if (!MinProperty)
			{
				throw std::runtime_error(std::format("Property 'min{}' doesn't exist in DT_PalDropItem, Pal Schema needs an update.", Index));
			}

			auto ItemId = loot.at("ItemId");
			auto DropChance = loot.at("DropChance");
			auto Min = loot.at("Min");
			auto Max = loot.at("Max");

			PropertyHelper::CopyJsonValueToContainer(NpcDropItemData, ItemIdProperty, ItemId);
			PropertyHelper::CopyJsonValueToContainer(NpcDropItemData, RateProperty, DropChance);
			PropertyHelper::CopyJsonValueToContainer(NpcDropItemData, MinProperty, Min);
			PropertyHelper::CopyJsonValueToContainer(NpcDropItemData, MaxProperty, Max);

			Index++;

			if (Index > 10)
			{
				break;
			}
		}

		auto RowName = std::format(STR("{}000"), CharacterId.ToString());
		n_dropItemTable->AddRow(FName(RowName, FNAME_Add), *reinterpret_cast<UECustom::FTableRowBase*>(NpcDropItemData));
	}

	//Add New NPC Translations
	void PalHumanModLoader::AddTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data)
	{
		if (Data.contains("Name"))
		{
			auto FixedCharacterId = std::format(STR("HUMAN_NAME_{}"), CharacterId.ToString());
			auto TranslationRowStruct = n_npcNameTable->GetRowStruct().Get();
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

				n_npcNameTable->AddRow(FName(FixedCharacterId, FNAME_Add), *reinterpret_cast<UECustom::FTableRowBase*>(TranslationRowData));
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
			auto TranslationRowStruct = n_npcNameTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = n_npcNameTable->FindRowUnchecked(FName(FixedCharacterId, FNAME_Add));
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
			if (n_npcTalkFlowTable)
			{
				n_npcTalkFlowTable->AddRow(CharacterId, NpcTalkFlowClassDataRow);
				if (!n_npcTalkFlowTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
			}
			else
			{
				PS::Log<RC::LogLevel::Warning>(STR("npcTalkFlowTable not found, skipping adding talk flow for {}\n"), CharacterId.ToString());
				addSucceeded = false;
			}
		} else {
			FPalNPCTalkFlowClassDataRow NpcTalkFlowClassDataRow {STR("/Game/Pal/Blueprint/FlowGraph/NPCTalkFlow/Graph/FABP_CommonItemShop_WithoutSell.FABP_CommonItemShop_WithoutSell")};
			if (n_npcTalkFlowTable)
			{
				n_npcTalkFlowTable->AddRow(CharacterId, NpcTalkFlowClassDataRow);
				if (!n_npcTalkFlowTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
			}
			else
			{
				PS::Log<RC::LogLevel::Warning>(STR("npcTalkFlowTable not found, skipping adding talk flow for {}\n"), CharacterId.ToString());
				addSucceeded = false;
			}
		}

		auto ShopTableId = RC::to_generic_string(Data.at("ShopTableId").get<std::string>());
		if (n_ItemShopLotteryDataTable)
		{
			FPalItemShopLotteryEntry entry{};
			entry.ShopGroupName = CharacterId;
			entry.Weight = 100;

			FPalItemShopLotteryDataRow row{};
			row.lotteryDataArray.Add(entry);

			n_ItemShopLotteryDataTable->AddRow(FName(ShopTableId, FNAME_Add), row);
			if (!n_ItemShopLotteryDataTable->FindRowUnchecked(FName(ShopTableId, FNAME_Add))) addSucceeded = false;
		}
		else
		{
			PS::Log<RC::LogLevel::Warning>(STR("ItemShopLotteryDataTable not found, skipping adding lottery row for {}\n"), CharacterId.ToString());
			addSucceeded = false;
		}
		if (n_ItemShopCreateDataTable)
		{
			auto CreateRowJson = nlohmann::json::object();
			CreateRowJson["productDataArray"] = Data.at("Items");

			auto RowKeyName = CharacterId;
			auto ExistingCreateRow = n_ItemShopCreateDataTable->FindRowUnchecked(RowKeyName);
			auto CreateRowStruct = n_ItemShopCreateDataTable->GetRowStruct().Get();
			if (ExistingCreateRow)
			{
				try
				{
					for (auto& Property : CreateRowStruct->ForEachPropertyInChain())
					{
						auto PropertyName = RC::to_string(Property->GetName());
						if (CreateRowJson.contains(PropertyName))
						{
							PropertyHelper::CopyJsonValueToContainer(ExistingCreateRow, Property, CreateRowJson.at(PropertyName));
						}
					}
					if (!n_ItemShopCreateDataTable->FindRowUnchecked(RowKeyName)) addSucceeded = false;
				}
				catch (const std::exception& e)
				{
					PS::Log<RC::LogLevel::Error>(STR("Failed to modify Row '{}' in {}: {}\n"), RowKeyName.ToString(), n_ItemShopCreateDataTable->GetFullName(), RC::to_generic_string(e.what()));
					addSucceeded = false;
				}
			}
			else
			{
				auto RowData = FMemory::Malloc(CreateRowStruct->GetStructureSize());
				CreateRowStruct->InitializeStruct(RowData);
				try
				{
					for (auto& Property : CreateRowStruct->ForEachPropertyInChain())
					{
						auto PropertyName = RC::to_string(Property->GetName());
						if (CreateRowJson.contains(PropertyName))
						{
							PropertyHelper::CopyJsonValueToContainer(RowData, Property, CreateRowJson.at(PropertyName));
						}
					}
					n_ItemShopCreateDataTable->AddRow(RowKeyName, *reinterpret_cast<UECustom::FTableRowBase*>(RowData));
					if (!n_ItemShopCreateDataTable->FindRowUnchecked(RowKeyName)) addSucceeded = false;
				}
				catch (const std::exception& e)
				{
					FMemory::Free(RowData);
					PS::Log<RC::LogLevel::Error>(STR("Failed to add Row '{}' to {}: {}\n"), RowKeyName.ToString(), n_ItemShopCreateDataTable->GetFullName(), RC::to_generic_string(e.what()));
					addSucceeded = false;
				}
			}
		}
		else
		{
			PS::Log<RC::LogLevel::Warning>(STR("ItemShopCreateDataTable not found, skipping adding create-data row for {}\n"), CharacterId.ToString());
			addSucceeded = false;
		}
		if (n_ItemShopSettingDataTable)
		{
			auto CurrencyStr = RC::to_generic_string(Data.at("Currency").get<std::string>());
			FPalItemShopSettingDataRow settingRow{ CurrencyStr };

			auto ExistingRow = n_ItemShopSettingDataTable->FindRowUnchecked(CharacterId);
			if (ExistingRow)
			{
				auto RowStruct = n_ItemShopSettingDataTable->GetRowStruct().Get();
				auto CurrencyProp = RowStruct->GetPropertyByName(STR("CurrencyItemID"));
				if (CurrencyProp)
				{
					PropertyHelper::CopyJsonValueToContainer(ExistingRow, CurrencyProp, CurrencyStr);
				}
				else
				{
					PS::Log<RC::LogLevel::Warning>(STR("CurrencyItemID property not found in ItemShopSettingData row struct, skipping update for {}\n"), CharacterId.ToString());
				}
				if (!n_ItemShopSettingDataTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
			}
			else
			{
				n_ItemShopSettingDataTable->AddRow(CharacterId, settingRow);
				if (!n_ItemShopSettingDataTable->FindRowUnchecked(CharacterId)) addSucceeded = false;
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