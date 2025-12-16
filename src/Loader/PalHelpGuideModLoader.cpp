#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "SDK/Structs/Custom/FManagedStruct.h"
#include "SDK/Classes/PalNoteDataAsset.h"
#include "SDK/Classes/PalNoteData.h"
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

		m_helpGuideMasterDataTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/HelpGuide/DT_HelpGuideMasterDataTable.DT_HelpGuideMasterDataTable"));

		m_helpGuideDescTextTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_HelpGuideDescText.DT_HelpGuideDescText"));

		m_helpGuideTextureDataTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
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
			}
		}
	}

	void PalHelpGuideModLoader::Add(const RC::Unreal::FName& NoteId, const nlohmann::json& Data)
	{
		if (NoteId == NAME_None)
		{
			throw std::runtime_error("ID was set to None");
		}

		UClass* PalNoteDataClass = UPalNoteData::StaticClass();
		if (!PalNoteDataClass)
        {
            throw std::runtime_error("Failed to find PalNoteData class. The class may have been renamed or removed.");
        }

        if (!PalNoteDataClass->GetPropertyByNameInChain(STR("TextId_Description")))
        {
            throw std::runtime_error("Property 'TextId_Description' has changed in DA_HelpGuideDataAsset. Update to Pal Schema is needed.");
        }

        if (!PalNoteDataClass->GetPropertyByNameInChain(STR("Texture")))
		{
			throw std::runtime_error("Property 'Texture' has changed in DA_HelpGuideDataAsset. Update to Pal Schema is needed.");
		}

		FStaticConstructObjectParameters ConstructParams(PalNoteDataClass, m_helpGuideDataAsset);
		ConstructParams.Name = NAME_None;

		auto NoteData = UObjectGlobals::StaticConstructObject<UPalNoteData*>(ConstructParams);

		// Set the TextId_Description to the NoteId by default
		auto TextIdProperty = NoteData->GetValuePtrByPropertyNameInChain<FName>(STR("TextId_Description"));
		if (TextIdProperty)
		{
			*TextIdProperty = NoteId;
		}

        if (Data.contains("Texture"))
        {
            auto TextureProperty = NoteData->GetPropertyByNameInChain(STR("Texture"));
            PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(NoteData), TextureProperty, Data.at("Texture"));
        }

		m_helpGuideDataAsset->NoteDataMap.Add(NoteId, NoteData);
	}

	void PalHelpGuideModLoader::Edit(const RC::Unreal::FName& NoteId, UPalNoteData* NoteData, const nlohmann::json& Data)
	{
        for (FProperty* Property : TFieldRange<FProperty>(NoteData->GetClassPrivate(), EFieldIterationFlags::IncludeSuper))
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
        auto DescriptionProperty = TableRowStruct->GetPropertyByNameInChain(STR("TextId_Description"));
        if (!DescriptionProperty)
        {
            PS::Log<RC::LogLevel::Error>(STR("Failed to add MasterData '{}': Property 'TextId_Description' doesn't exist in {}\n"), NoteId.ToString(), m_helpGuideMasterDataTable->GetName());
            return;
        }

		if (TableRow)
		{
            auto Description = DescriptionProperty->ContainerPtrToValuePtr<FName>(TableRow);
            *Description = NoteId;
		}
		else
		{
			auto RowData = FManagedStruct(TableRowStruct);
            auto Description = DescriptionProperty->ContainerPtrToValuePtr<FName>(RowData.GetData());
            *Description = NoteId;

            m_helpGuideMasterDataTable->AddRow(NoteId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(RowData.GetData()));
		}
	}

	void PalHelpGuideModLoader::AddOrEditDescText(const RC::Unreal::FName& NoteId, const nlohmann::json& Data)
	{
		if (!m_helpGuideDescTextTable) return;

        std::string Title = "";
        std::string Description = "";

        if (Data.contains("Title") && Data.at("Title").is_string())
        {
            Title = Data.at("Title").get<std::string>();
        }

        if (Data.contains("Description") && Data.at("Description").is_string())
        {
            Description = Data.at("Description").get<std::string>();
        }

        if (Title == "")
        {
            Title = RC::to_string(NoteId.ToString());
        }

        if (!Title.ends_with("\r\n\r\n"))
        {
            Title = std::format("{}\r\n\r\n", Title);
        }

		auto TableRow = m_helpGuideDescTextTable->FindRowUnchecked(NoteId);
		auto TableRowStruct = m_helpGuideDescTextTable->GetRowStruct().Get();

		if (TableRow)
		{
            auto TextDataProperty = TableRowStruct->GetPropertyByNameInChain(STR("TextData"));
            if (TextDataProperty)
            {
                auto TextData = TextDataProperty->ContainerPtrToValuePtr<FText>(TableRow);
                auto FinalDescription = std::format("{}{}", Title, Description);
                auto FinalDescriptionWide = RC::to_generic_string(FinalDescription);
                *TextData = FText(FinalDescriptionWide.c_str());
            }
		}
		else
		{
            auto RowData = FManagedStruct(TableRowStruct);

            auto TextDataProperty = TableRowStruct->GetPropertyByNameInChain(STR("TextData"));
            if (TextDataProperty)
            {
                auto TextData = TextDataProperty->ContainerPtrToValuePtr<FText>(RowData.GetData());
                auto FinalDescription = std::format("{}{}", Title, Description);
                auto FinalDescriptionWide = RC::to_generic_string(FinalDescription);
                *TextData = FText(FinalDescriptionWide.c_str());
            }

            m_helpGuideDescTextTable->AddRow(NoteId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(RowData.GetData()));
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
