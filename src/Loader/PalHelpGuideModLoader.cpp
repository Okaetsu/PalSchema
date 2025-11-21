#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UClass.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "SDK/Classes/PalNoteDataAsset.h"
#include "SDK/Classes/PalNoteData.h"
#include "SDK/Classes/UDataTable.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Helpers/String.hpp"
#include "Utility/Logging.h"
#include "Loader/PalHelpGuideModLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalHelpGuideModLoader::PalHelpGuideModLoader() : PalModLoaderBase("helpguide") {}

	PalHelpGuideModLoader::~PalHelpGuideModLoader() {}

	void PalHelpGuideModLoader::Initialize()
	{
		m_helpGuideDataAsset = UObjectGlobals::StaticFindObject<UPalNoteDataAsset*>(nullptr, nullptr,
			STR("/Game/Pal/DataAsset/HelpGuide/DA_HelpGuideDataAsset.DA_HelpGuideDataAsset"));

		m_helpGuideMasterDataTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/HelpGuide/DT_HelpGuideMasterDataTable.DT_HelpGuideMasterDataTable"));

		m_helpGuideDescTextTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_HelpGuideDescText.DT_HelpGuideDescText"));

		m_helpGuideTextureDataTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/HelpGuide/DT_HelpGuideTextureDataTable.DT_HelpGuideTextureDataTable"));

		PS::Log<RC::LogLevel::Verbose>(STR("Initialized HelpGuideModLoader\n"));
	}

	void PalHelpGuideModLoader::Load(const nlohmann::json& Data)
	{
		for (auto& [Key, Value] : Data.items())
		{
			auto NoteId = FName(RC::to_generic_string(Key), FNAME_Add);
			
			if (Value.is_null())
			{
				// Delete from all sources
				auto Row = m_helpGuideDataAsset->NoteDataMap.Find(NoteId);
				if (Row)
				{
					m_helpGuideDataAsset->NoteDataMap.Remove(NoteId);
				}
				
				DeleteRelatedData(NoteId);
				PS::Log<RC::LogLevel::Normal>(STR("Deleted Help Guide '{}'\n"), NoteId.ToString());
			}
			else
			{
				auto Row = m_helpGuideDataAsset->NoteDataMap.Find(NoteId);
				if (Row)
				{
					Edit(NoteId, *Row, Value);
					PS::Log<RC::LogLevel::Normal>(STR("Modified Help Guide '{}'\n"), NoteId.ToString());
				}
				else
				{
					Add(NoteId, Value);
					PS::Log<RC::LogLevel::Normal>(STR("Added Help Guide '{}'\n"), NoteId.ToString());
				}

				// Always update the DataTables
				AddOrEditMasterData(NoteId, Value);
				AddOrEditDescText(NoteId, Value);
				AddOrEditTextureData(NoteId, Value);
			}
		}
	}

	void PalHelpGuideModLoader::Add(const RC::Unreal::FName& NoteId, const nlohmann::json& Data)
	{
		if (NoteId == NAME_None)
		{
			throw std::runtime_error("ID was set to None");
		}

		// Find the actual PalNoteData class from the game
		UClass* DatabaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Pal.PalNoteData"));
		
		if (!DatabaseClass)
		{
			throw std::runtime_error("Failed to find PalNoteData class. The class may have been renamed or removed.");
		}

		if (!DatabaseClass->GetPropertyByNameInChain(STR("TextId_Description")))
		{
			throw std::runtime_error("Property 'TextId_Description' has changed in DA_HelpGuideDataAsset. Update to Pal Schema is needed.");
		}

		if (!DatabaseClass->GetPropertyByNameInChain(STR("Texture")))
		{
			throw std::runtime_error("Property 'Texture' has changed in DA_HelpGuideDataAsset. Update to Pal Schema is needed.");
		}

		FStaticConstructObjectParameters ConstructParams(DatabaseClass, m_helpGuideDataAsset);
		ConstructParams.Name = NAME_None;

		auto NoteData = UObjectGlobals::StaticConstructObject<UPalNoteData*>(ConstructParams);

		// Set the TextId_Description to the NoteId by default
		auto TextIdProperty = NoteData->GetValuePtrByPropertyNameInChain<FName>(STR("TextId_Description"));
		if (TextIdProperty)
		{
			*TextIdProperty = NoteId;
		}

		// Iterate through all properties and copy from JSON
		for (auto& Property : DatabaseClass->ForEachPropertyInChain())
		{
			auto PropertyName = RC::to_string(Property->GetName());
			if (Data.contains(PropertyName))
			{
				PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(NoteData), Property, Data.at(PropertyName));
			}
		}

		m_helpGuideDataAsset->NoteDataMap.Add(NoteId, NoteData);
	}

	void PalHelpGuideModLoader::Edit(const RC::Unreal::FName& NoteId, UPalNoteData* NoteData, const nlohmann::json& Data)
	{
		for (auto& Property : NoteData->GetClassPrivate()->ForEachPropertyInChain())
		{
			auto PropertyName = RC::to_string(Property->GetName());
			if (Data.contains(PropertyName))
			{
				PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(NoteData), Property, Data.at(PropertyName));
			}
		}
	}

	void PalHelpGuideModLoader::AddOrEditMasterData(const RC::Unreal::FName& NoteId, const nlohmann::json& Data)
	{
		if (!m_helpGuideMasterDataTable) return;

		auto TableRow = m_helpGuideMasterDataTable->FindRowUnchecked(NoteId);
		auto TableRowStruct = m_helpGuideMasterDataTable->GetRowStruct().Get();

		if (TableRow)
		{
			// Edit existing row
			try
			{
				if (Data.contains("TextId_Description"))
				{
					auto Property = TableRowStruct->GetPropertyByNameInChain(STR("TextId_Description"));
					if (Property)
					{
						PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at("TextId_Description"));
					}
				}
			}
			catch (const std::exception& e)
			{
				PS::Log<RC::LogLevel::Error>(STR("Failed to modify MasterData '{}': {}\n"), NoteId.ToString(), RC::to_generic_string(e.what()));
			}
		}
		else
		{
			// Add new row
			auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
			TableRowStruct->InitializeStruct(RowData);

			try
			{
				// Set TextId_Description (default to NoteId if not provided)
				auto Property = TableRowStruct->GetPropertyByNameInChain(STR("TextId_Description"));
				if (Property)
				{
					if (Data.contains("TextId_Description"))
					{
						PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at("TextId_Description"));
					}
					else
					{
						// Default to NoteId
						PropertyHelper::CopyJsonValueToContainer(RowData, Property, RC::to_string(NoteId.ToString()));
					}
				}

				m_helpGuideMasterDataTable->AddRow(NoteId, *reinterpret_cast<UECustom::FTableRowBase*>(RowData));
			}
			catch (const std::exception& e)
			{
				FMemory::Free(RowData);
				PS::Log<RC::LogLevel::Error>(STR("Failed to add MasterData '{}': {}\n"), NoteId.ToString(), RC::to_generic_string(e.what()));
			}
		}
	}

	void PalHelpGuideModLoader::AddOrEditDescText(const RC::Unreal::FName& NoteId, const nlohmann::json& Data)
	{
		if (!m_helpGuideDescTextTable) return;
		if (!Data.contains("TextData")) return; // Skip if no text data provided

		auto TableRow = m_helpGuideDescTextTable->FindRowUnchecked(NoteId);
		auto TableRowStruct = m_helpGuideDescTextTable->GetRowStruct().Get();

		if (TableRow)
		{
			// Edit existing row
			try
			{
				auto Property = TableRowStruct->GetPropertyByNameInChain(STR("TextData"));
				if (Property)
				{
					PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at("TextData"));
				}
			}
			catch (const std::exception& e)
			{
				PS::Log<RC::LogLevel::Error>(STR("Failed to modify DescText '{}': {}\n"), NoteId.ToString(), RC::to_generic_string(e.what()));
			}
		}
		else
		{
			// Add new row
			auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
			TableRowStruct->InitializeStruct(RowData);

			try
			{
				auto Property = TableRowStruct->GetPropertyByNameInChain(STR("TextData"));
				if (Property)
				{
					PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at("TextData"));
				}

				m_helpGuideDescTextTable->AddRow(NoteId, *reinterpret_cast<UECustom::FTableRowBase*>(RowData));
			}
			catch (const std::exception& e)
			{
				FMemory::Free(RowData);
				PS::Log<RC::LogLevel::Error>(STR("Failed to add DescText '{}': {}\n"), NoteId.ToString(), RC::to_generic_string(e.what()));
			}
		}
	}

	void PalHelpGuideModLoader::AddOrEditTextureData(const RC::Unreal::FName& NoteId, const nlohmann::json& Data)
	{
		if (!m_helpGuideTextureDataTable) return;
		if (!Data.contains("Texture")) return; // Skip if no texture provided

		auto TableRow = m_helpGuideTextureDataTable->FindRowUnchecked(NoteId);
		auto TableRowStruct = m_helpGuideTextureDataTable->GetRowStruct().Get();

		if (TableRow)
		{
			// Edit existing row
			try
			{
				auto Property = TableRowStruct->GetPropertyByNameInChain(STR("Texture"));
				if (Property)
				{
					PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at("Texture"));
				}
			}
			catch (const std::exception& e)
			{
				PS::Log<RC::LogLevel::Error>(STR("Failed to modify TextureData '{}': {}\n"), NoteId.ToString(), RC::to_generic_string(e.what()));
			}
		}
		else
		{
			// Add new row
			auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
			TableRowStruct->InitializeStruct(RowData);

			try
			{
				auto Property = TableRowStruct->GetPropertyByNameInChain(STR("Texture"));
				if (Property)
				{
					PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at("Texture"));
				}

				m_helpGuideTextureDataTable->AddRow(NoteId, *reinterpret_cast<UECustom::FTableRowBase*>(RowData));
			}
			catch (const std::exception& e)
			{
				FMemory::Free(RowData);
				PS::Log<RC::LogLevel::Error>(STR("Failed to add TextureData '{}': {}\n"), NoteId.ToString(), RC::to_generic_string(e.what()));
			}
		}
	}

	void PalHelpGuideModLoader::DeleteRelatedData(const RC::Unreal::FName& NoteId)
	{
		if (m_helpGuideMasterDataTable)
		{
			m_helpGuideMasterDataTable->RemoveRow(NoteId);
		}

		if (m_helpGuideDescTextTable)
		{
			m_helpGuideDescTextTable->RemoveRow(NoteId);
		}

		if (m_helpGuideTextureDataTable)
		{
			m_helpGuideTextureDataTable->RemoveRow(NoteId);
		}
	}
}
