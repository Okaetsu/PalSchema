#include "Unreal/UObjectGlobals.hpp"
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

		/* Long & Short descriptions seem to be using default human description.
		if (Data.contains("ShortDescription"))
		{
			auto FixedCharacterId = std::format(STR("PAL_SHORT_DESC_{}"), CharacterId.ToString());
			auto TranslationRowStruct = n_palShortDescTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto TranslationRowData = FMemory::Malloc(TranslationRowStruct->GetStructureSize());
				TranslationRowStruct->InitializeStruct(TranslationRowData);

				try
				{
					PropertyHelper::CopyJsonValueToContainer(TranslationRowData, TextProperty, Data.at("ShortDescription"));
				}
				catch (const std::exception& e)
				{
					FMemory::Free(TranslationRowData);
					throw std::runtime_error(e.what());
				}

				n_palShortDescTable->AddRow(FName(FixedCharacterId, FNAME_Add), *reinterpret_cast<UECustom::FTableRowBase*>(TranslationRowData));
			}
		}

		if (Data.contains("LongDescription"))
		{
			auto FixedCharacterId = std::format(STR("PAL_LONG_DESC_{}"), CharacterId.ToString());
			auto TranslationRowStruct = n_palLongDescTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto TranslationRowData = FMemory::Malloc(TranslationRowStruct->GetStructureSize());
				TranslationRowStruct->InitializeStruct(TranslationRowData);

				try
				{
					PropertyHelper::CopyJsonValueToContainer(TranslationRowData, TextProperty, Data.at("LongDescription"));
				}
				catch (const std::exception& e)
				{
					FMemory::Free(TranslationRowData);
					throw std::runtime_error(e.what());
				}

				n_palLongDescTable->AddRow(FName(FixedCharacterId, FNAME_Add), *reinterpret_cast<UECustom::FTableRowBase*>(TranslationRowData));
			}
		}
		*/
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

		/*
		if (Data.contains("ShortDescription"))
		{
			auto FixedCharacterId = std::format(STR("PAL_SHORT_DESC_{}"), CharacterId.ToString());
			auto TranslationRowStruct = n_palShortDescTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = n_palShortDescTable->FindRowUnchecked(FName(FixedCharacterId, FNAME_Add));
				if (Row)
				{
					PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data.at("ShortDescription"));
				}
			}
		}

		if (Data.contains("LongDescription"))
		{
			auto FixedCharacterId = std::format(STR("PAL_LONG_DESC_{}"), CharacterId.ToString());
			auto TranslationRowStruct = n_palLongDescTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = n_palLongDescTable->FindRowUnchecked(FName(FixedCharacterId, FNAME_Add));
				if (Row)
				{
					PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data.at("LongDescription"));
				}
			}
		}
		*/
	}
}