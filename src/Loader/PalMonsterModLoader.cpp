#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/KismetInternationalizationLibrary.h"
#include "SDK/Structs/FPalCharacterIconDataRow.h"
#include "SDK/Structs/FPalBPClassDataRow.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include "Loader/PalMonsterModLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalMonsterModLoader::PalMonsterModLoader() : PalModLoaderBase("pals") {}

	PalMonsterModLoader::~PalMonsterModLoader() {}

	void PalMonsterModLoader::Initialize()
	{
		m_dataTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Character/DT_PalMonsterParameter.DT_PalMonsterParameter"));

		m_iconDataTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr, 
			STR("/Game/Pal/DataTable/Character/DT_PalCharacterIconDataTable.DT_PalCharacterIconDataTable"));

		m_palBpClassTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Character/DT_PalBPClass.DT_PalBPClass"));

		m_wazaMasterLevelTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Waza/DT_WazaMasterLevel.DT_WazaMasterLevel"));

		m_palDropItemTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Character/DT_PalDropItem.DT_PalDropItem"));

		m_palNameTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_PalNameText.DT_PalNameText"));

		m_palShortDescTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_PalShortDescriptionText.DT_PalShortDescriptionText"));

		m_palLongDescTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_PalLongDescriptionText.DT_PalLongDescriptionText"));

        PS::Log<LogLevel::Verbose>(STR("Initialized MonsterModLoader\n"));
	}

	void PalMonsterModLoader::Load(const nlohmann::json& json)
	{
		for (auto& [character_id, properties] : json.items())
		{
			auto CharacterId = FName(RC::to_generic_string(character_id), FNAME_Add);
			auto TableRow = m_dataTable->FindRowUnchecked(CharacterId);
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

	void PalMonsterModLoader::Add(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
	{
		auto MonsterRowStruct = m_dataTable->GetRowStruct().Get();
		auto MonsterRowData = FMemory::Malloc(MonsterRowStruct->GetStructureSize());
		MonsterRowStruct->InitializeStruct(MonsterRowData);

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
			else if (KeyName == STR("AbilitiesByLevel"))
			{
				AddAbilities(CharacterId, value);
			}
			else if (KeyName == STR("Loot"))
			{
				AddLoot(CharacterId, value);
			}
			else
			{
				auto Property = MonsterRowStruct->GetPropertyByName(KeyName.c_str());
				if (Property)
				{
					PropertyHelper::CopyJsonValueToContainer(MonsterRowData, Property, value);
				}
			}
		}

        // IsPal is false by default, so we set it to true here, because PalMonsterModLoader is meant for Pals specifically.
        // There's some side effects that can happen if it's set to false, such as the Pal not showing up in Paldeck.
        auto IsPalProp = MonsterRowStruct->GetPropertyByName(STR("IsPal"));
        if (IsPalProp)
        {
            *IsPalProp->ContainerPtrToValuePtr<bool>(MonsterRowData) = true;
        }

		m_dataTable->AddRow(CharacterId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(MonsterRowData));

		AddTranslations(CharacterId, properties);

        PS::Log<RC::LogLevel::Normal>(STR("Added new Pal '{}'\n"), CharacterId.ToString());
	}

	void PalMonsterModLoader::Edit(uint8_t* TableRow, const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
	{
		auto RowStruct = m_dataTable->GetRowStruct().Get();
		for (auto& [key, value] : properties.items())
		{
			auto KeyName = RC::to_generic_string(key);
			if (KeyName == STR("IconAssetPath"))
			{
				auto IconTableRow = std::bit_cast<FPalCharacterIconDataRow*>(m_iconDataTable->FindRowUnchecked(CharacterId));
				if (IconTableRow)
				{
					auto IconPath = RC::to_generic_string(value.get<std::string>());
					IconTableRow->Icon = UECustom::TSoftObjectPtr<UECustom::UTexture2D>(UECustom::FSoftObjectPath(IconPath));
				}
			}
			else if (KeyName == STR("ActorClassPath"))
			{
				auto BlueprintTableRow = std::bit_cast<FPalBPClassDataRow*>(m_palBpClassTable->FindRowUnchecked(CharacterId));
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

	void PalMonsterModLoader::AddIcon(const RC::Unreal::FName& CharacterId, const RC::StringType& IconPath)
	{
		FPalCharacterIconDataRow IconDataRow{ IconPath };
		m_iconDataTable->AddRow(CharacterId, IconDataRow);
	}

	void PalMonsterModLoader::AddBlueprint(const RC::Unreal::FName& CharacterId, const RC::StringType& BlueprintPath)
	{
		FPalBPClassDataRow BlueprintDataRow{ BlueprintPath };
		m_palBpClassTable->AddRow(CharacterId, BlueprintDataRow);
	}

	void PalMonsterModLoader::AddAbilities(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
	{
		auto RowStruct = m_wazaMasterLevelTable->GetRowStruct().Get();

		for (auto& [key, value] : properties.items())
		{
			if (!value.contains("WazaID"))
			{
				PS::Log<RC::LogLevel::Error>(STR("WazaID was not specified in {}, skipping ability entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!value.contains("Level"))
			{
				PS::Log<RC::LogLevel::Error>(STR("Level was not specified in {}, skipping ability entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!value.at("WazaID").is_string())
			{
				PS::Log<RC::LogLevel::Error>(STR("WazaID in {} must be a string, skipping ability entry.\n"), CharacterId.ToString());
				continue;
			}

			if (!value.at("Level").is_number_integer())
			{
				PS::Log<RC::LogLevel::Error>(STR("Level in {} must be an integer, skipping ability entry.\n"), CharacterId.ToString());
				continue;
			}

			auto Level = value.at("Level").get<int>();

			auto WazaMasterLevelData = FMemory::Malloc(RowStruct->GetStructureSize());
			RowStruct->InitializeStruct(WazaMasterLevelData);

			auto PalIDProperty = RowStruct->GetPropertyByName(STR("PalID"));
			if (PalIDProperty)
			{
				FMemory::Memcpy(PalIDProperty->ContainerPtrToValuePtr<void>(WazaMasterLevelData), &CharacterId, sizeof(FName));
			}

			auto WazaIDProperty = RowStruct->GetPropertyByName(STR("WazaID"));
			if (WazaIDProperty)
			{
				PropertyHelper::CopyJsonValueToContainer(WazaMasterLevelData, WazaIDProperty, value.at("WazaID"));
			}

			auto LevelProperty = RowStruct->GetPropertyByName(STR("Level"));
			if (LevelProperty)
			{
				FMemory::Memcpy(LevelProperty->ContainerPtrToValuePtr<void>(WazaMasterLevelData), &Level, sizeof(int));
			}

			auto NewRow = reinterpret_cast<RC::Unreal::FTableRowBase*>(WazaMasterLevelData);
			auto NewRowName = std::format(STR("{}{}"), CharacterId.ToString(), Level);

			m_wazaMasterLevelTable->AddRow(FName(NewRowName, FNAME_Add), *NewRow);
		}
	}

	void PalMonsterModLoader::AddLoot(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties)
	{
		auto RowStruct = m_palDropItemTable->GetRowStruct().Get();

		auto PalDropItemData = FMemory::Malloc(RowStruct->GetStructureSize());
		RowStruct->InitializeStruct(PalDropItemData);

		auto CharacterIdProperty = RowStruct->GetPropertyByName(STR("CharacterId"));
		if (!CharacterIdProperty)
		{
			FMemory::Free(PalDropItemData);
			throw std::runtime_error("Property CharacterId doesn't exist in DT_PalDropItem, which means you should wait for an update as something in the table has changed.");
		}

		FMemory::Memcpy(CharacterIdProperty->ContainerPtrToValuePtr<void>(PalDropItemData), &CharacterId, sizeof(FName));

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

			PropertyHelper::CopyJsonValueToContainer(PalDropItemData, ItemIdProperty, ItemId);
			PropertyHelper::CopyJsonValueToContainer(PalDropItemData, RateProperty, DropChance);
			PropertyHelper::CopyJsonValueToContainer(PalDropItemData, MinProperty, Min);
			PropertyHelper::CopyJsonValueToContainer(PalDropItemData, MaxProperty, Max);

			Index++;

			if (Index > 10)
			{
				break;
			}
		}

		auto RowName = std::format(STR("{}000"), CharacterId.ToString());
		m_palDropItemTable->AddRow(FName(RowName, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(PalDropItemData));
	}

	void PalMonsterModLoader::AddTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data)
	{
		if (Data.contains("Name"))
		{
			auto FixedCharacterId = std::format(STR("PAL_NAME_{}"), CharacterId.ToString());
			auto TranslationRowStruct = m_palNameTable->GetRowStruct().Get();
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

				m_palNameTable->AddRow(FName(FixedCharacterId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(TranslationRowData));
			}
		}

		if (Data.contains("ShortDescription"))
		{
			auto FixedCharacterId = std::format(STR("PAL_SHORT_DESC_{}"), CharacterId.ToString());
			auto TranslationRowStruct = m_palShortDescTable->GetRowStruct().Get();
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

				m_palShortDescTable->AddRow(FName(FixedCharacterId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(TranslationRowData));
			}
		}

		if (Data.contains("LongDescription"))
		{
			auto FixedCharacterId = std::format(STR("PAL_LONG_DESC_{}"), CharacterId.ToString());
			auto TranslationRowStruct = m_palLongDescTable->GetRowStruct().Get();
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

				m_palLongDescTable->AddRow(FName(FixedCharacterId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(TranslationRowData));
			}
		}
	}

	void PalMonsterModLoader::EditTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data)
	{
		if (Data.contains("Name"))
		{
			auto FixedCharacterId = std::format(STR("PAL_NAME_{}"), CharacterId.ToString());
			auto TranslationRowStruct = m_palNameTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = m_palNameTable->FindRowUnchecked(FName(FixedCharacterId, FNAME_Add));
				if (Row)
				{
					PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data.at("Name"));
				}
			}
		}

		if (Data.contains("ShortDescription"))
		{
			auto FixedCharacterId = std::format(STR("PAL_SHORT_DESC_{}"), CharacterId.ToString());
			auto TranslationRowStruct = m_palShortDescTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = m_palShortDescTable->FindRowUnchecked(FName(FixedCharacterId, FNAME_Add));
				if (Row)
				{
					PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data.at("ShortDescription"));
				}
			}
		}

		if (Data.contains("LongDescription"))
		{
			auto FixedCharacterId = std::format(STR("PAL_LONG_DESC_{}"), CharacterId.ToString());
			auto TranslationRowStruct = m_palLongDescTable->GetRowStruct().Get();
			auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
			if (TextProperty)
			{
				auto Row = m_palLongDescTable->FindRowUnchecked(FName(FixedCharacterId, FNAME_Add));
				if (Row)
				{
					PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data.at("LongDescription"));
				}
			}
		}
	}
}