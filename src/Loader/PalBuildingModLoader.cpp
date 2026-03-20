#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "Helpers/String.hpp"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Utility/JsonHelpers.h"
#include "Loader/PalBuildingModLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalBuildingModLoader::PalBuildingModLoader() : PalModLoaderBase("buildings") {
        SetDisplayName(TEXT("Building Mod Loader"));
    }

	PalBuildingModLoader::~PalBuildingModLoader() {}

    void PalBuildingModLoader::OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase != EEngineLifecyclePhase::GameInstanceInit)
        {
            return;
        }

        PS::JsonHelpers::ParseJsonFilesInPath(loaderPath, [&](const nlohmann::json& data) {
            LoadBuildings(data);
        });
    }

    void PalBuildingModLoader::OnAutoReload(const RC::StringType& modName, const std::filesystem::path& modFilePath)
    {
        PS::JsonHelpers::ParseJsonFileInPath(modFilePath, [&](const nlohmann::json& data) {
            LoadBuildings(data);
        });
    }

    bool PalBuildingModLoader::CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase == EEngineLifecyclePhase::GameInstanceInit)
        {
            return true;
        }

        return false;
    }

    bool PalBuildingModLoader::OnInitialize()
    {
        try
        {
            m_mapObjectAssignData = GetDatatableByName("DT_MapObjectAssignData");
            m_mapObjectFarmCrop = GetDatatableByName("DT_MapObjectFarmCrop");
            m_mapObjectItemProductDataTable = GetDatatableByName("DT_MapObjectItemProductDataTable");
            m_mapObjectMasterDataTable = GetDatatableByName("DT_MapObjectMasterDataTable");
            m_mapObjectNameTable = GetDatatableByName("DT_MapObjectNameText");
            m_buildObjectDataTable = GetDatatableByName("DT_BuildObjectDataTable");
            m_buildObjectIconDataTable = GetDatatableByName("DT_BuildObjectIconDataTable");
            m_buildObjectDescTable = GetDatatableByName("DT_BuildObjectDescText");
            m_technologyRecipeUnlockTable = GetDatatableByName("DT_TechnologyRecipeUnlock");
            m_technologyNameTable = GetDatatableByName("DT_TechnologyNameText");
            m_technologyDescTable = GetDatatableByName("DT_TechnologyDescText");
        }
        catch (const std::exception& e)
        {
            PS::Log<LogLevel::Error>(STR("Unable to initialize {}, {}\n"), GetDisplayName(), RC::to_generic_string(e.what()));
            return false;
        }

        return true;
    }

    void PalBuildingModLoader::LoadBuildings(const nlohmann::json& data)
    {
        for (auto& [Key, Properties] : data.items())
        {
            auto BuildingId = FName(RC::to_generic_string(Key), FNAME_Add);
            auto TableRow = m_mapObjectMasterDataTable->FindRowUnchecked(BuildingId);
            if (TableRow)
            {
                PS::Log<LogLevel::Error>(STR("Editing of buildings should be done via raw tables instead\n"));
            }
            else
            {
                Add(BuildingId, Properties);
            }
        }
    }

	void PalBuildingModLoader::Add(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
	{
		if (!Data.contains("BlueprintClassName"))
		{
			throw std::runtime_error(RC::fmt("%S is missing the property 'BlueprintClassName'", BuildingId.ToString().c_str()));
		}

		if (!Data.contains("BlueprintClassSoft"))
		{
			throw std::runtime_error(RC::fmt("%S is missing the property 'BlueprintClassSoft'", BuildingId.ToString().c_str()));
		}

		if (!Data.contains("IconTexture"))
		{
			throw std::runtime_error(RC::fmt("%S is missing the property 'IconTexture'", BuildingId.ToString().c_str()));
		}

        auto TableRow = m_mapObjectMasterDataTable->FindRowUnchecked(BuildingId);
        auto TableRowStruct = m_mapObjectMasterDataTable->GetRowStruct().Get();
        if (TableRow)
        {
            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());

                    if (PropertyName == "Editor_RowNameHash")
                    {
                        continue;
                    }

                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at(PropertyName));
                    }
                }
            }
            catch (const std::exception& e)
            {
                PS::Log<LogLevel::Error>(STR("Failed to modify Row '{}' in {}: {}\n"), BuildingId.ToString(), m_mapObjectMasterDataTable->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
        else
        {
            auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
            TableRowStruct->InitializeStruct(RowData);
            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());

                    if (PropertyName == "Editor_RowNameHash")
                    {
                        continue;
                    }

                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at(PropertyName));
                    }
                }

                m_mapObjectMasterDataTable->AddRow(BuildingId, *static_cast<RC::Unreal::FTableRowBase*>(RowData));

                PS::Log<LogLevel::Normal>(STR("Added building '{}'\n"), BuildingId.ToString());
            }
            catch (const std::exception& e)
            {
                FMemory::Free(RowData);
                PS::Log<LogLevel::Error>(STR("Failed to add Row '{}' to {}: {}\n"), BuildingId.ToString(), m_mapObjectMasterDataTable->GetFullName(), RC::to_generic_string(e.what()));
            }
        }

		SetupIconData(BuildingId, Data);

        SetupTranslations(BuildingId, Data);

		if (Data.contains("BuildingData"))
		{
			SetupBuildData(BuildingId, Data.at("BuildingData"));
		}

		if (Data.contains("Assignments"))
		{
            SetupAssignments(BuildingId, Data.at("Assignments"));
		}

        if (Data.contains("Technology"))
        {
            SetupTechnologyData(BuildingId, Data.at("Technology"));
        }

		if (Data.contains("Crop"))
		{
			SetupCropData(BuildingId, Data.at("Crop"));
		}

		if (Data.contains("ItemProductData"))
		{
			SetupItemProductData(BuildingId, Data.at("ItemProductData"));
		}
	}

	void PalBuildingModLoader::Edit(uint8_t* ExistingRow, const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
	{

	}

	void PalBuildingModLoader::SetupBuildData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
	{
        auto TableRow = m_buildObjectDataTable->FindRowUnchecked(BuildingId);
        auto TableRowStruct = m_buildObjectDataTable->GetRowStruct().Get();
        if (TableRow)
        {
            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());

                    if (PropertyName == "RedialIndex")
                    {
                        // Deny editing of RedialIndex, if you see this comment, use raw tables instead if you really need to edit it.
                        continue;
                    }

                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at(PropertyName));
                    }
                }
            }
            catch (const std::exception& e)
            {
                PS::Log<LogLevel::Error>(STR("Failed to modify Row '{}' in {}: {}\n"), BuildingId.ToString(), m_buildObjectDataTable->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
        else
        {
            auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
            TableRowStruct->InitializeStruct(RowData);

            if (Data.contains("RedialIndex"))
            {
                PS::Log<LogLevel::Warning>(STR("When adding new buildings, including 'RedialIndex' will not do anything as this will be handled by PalSchema to avoid collisions. This is a warning and will not affect the loading of your building '{}'.\n"), BuildingId.ToString());
            }

            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());

                    // Let PalSchema handle Radial Index to avoid collisions with the build wheel in-game.
                    if (PropertyName == "RedialIndex")
                    {
                        auto RadialIndex = GetNextRadialIndex();
                        FMemory::Memcpy(Property->ContainerPtrToValuePtr<void>(RowData), &RadialIndex, sizeof(int));
                        continue;
                    }

                    if (PropertyName == "MapObjectId")
                    {
                        FMemory::Memcpy(Property->ContainerPtrToValuePtr<void>(RowData), &BuildingId, sizeof(FName));
                        continue;
                    }

                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at(PropertyName));
                    }
                }

                m_buildObjectDataTable->AddRow(BuildingId, *static_cast<RC::Unreal::FTableRowBase*>(RowData));
            }
            catch (const std::exception& e)
            {
                FMemory::Free(RowData);
                PS::Log<LogLevel::Error>(STR("Failed to add Row '{}' to {}: {}\n"), BuildingId.ToString(), m_buildObjectDataTable->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
	}

	void PalBuildingModLoader::SetupIconData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
	{
        if (Data.contains("IconTexture"))
        {
            auto TableRow = m_buildObjectIconDataTable->FindRowUnchecked(BuildingId);
            auto TableRowStruct = m_buildObjectIconDataTable->GetRowStruct().Get();
            if (TableRow)
            {
                try
                {
                    auto Property = TableRowStruct->GetPropertyByNameInChain(STR("SoftIcon"));
                    if (Property)
                    {
                        PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at("IconTexture"));
                    }
                }
                catch (const std::exception& e)
                {
                    PS::Log<LogLevel::Error>(STR("Failed to modify Icon '{}' in {}: {}\n"), BuildingId.ToString(), m_buildObjectIconDataTable->GetFullName(), RC::to_generic_string(e.what()));
                }
            }
            else
            {
                auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
                TableRowStruct->InitializeStruct(RowData);
                try
                {
                    auto Property = TableRowStruct->GetPropertyByNameInChain(STR("SoftIcon"));
                    if (Property)
                    {
                        PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at("IconTexture"));
                        m_buildObjectIconDataTable->AddRow(BuildingId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(RowData));
                    }
                }
                catch (const std::exception& e)
                {
                    PS::Log<LogLevel::Error>(STR("Failed to add Icon for '{}' in {}: {}\n"), BuildingId.ToString(), m_buildObjectIconDataTable->GetFullName(), RC::to_generic_string(e.what()));
                }

                TableRowStruct->DestroyStruct(RowData);
                FMemory::Free(RowData);
            }
        }
	}

	void PalBuildingModLoader::SetupAssignments(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
	{
        if (!Data.is_array())
        {
            PS::Log<LogLevel::Error>(STR("Field 'Assignments' in {} must be an array of objects.\n"), BuildingId.ToString());
            return;
        }

        for (auto& Assignment : Data)
        {
            SetupAssignment(BuildingId, Assignment);
        }
	}

    void PalBuildingModLoader::SetupAssignment(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
    {
        if (!Data.contains("WorkSuitability"))
        {
            throw std::runtime_error(RC::fmt("Assignment in Row '%S' must contain a WorkSuitability field", BuildingId.ToString().c_str()));
        }

        if (!Data.contains("WorkType"))
        {
            throw std::runtime_error(RC::fmt("Assignment in Row '%S' must contain a WorkType field", BuildingId.ToString().c_str()));
        }

        if (!Data.contains("WorkActionType"))
        {
            throw std::runtime_error(RC::fmt("Assignment in Row '%S' must contain a WorkActionType field", BuildingId.ToString().c_str()));
        }

        auto TableRowStruct = m_mapObjectAssignData->GetRowStruct().Get();
        auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
        TableRowStruct->InitializeStruct(RowData);

        for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
        {
            auto PropertyName = RC::to_string(Property->GetName());
            if (Data.contains(PropertyName))
            {
                PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at(PropertyName));
            }
        }

        auto Suffix = GetAssignIDSuffixByWorkType(Data.at("WorkType"));
        auto RowFixedName = std::format(STR("{}{}"), BuildingId.ToString(), Suffix);
        m_mapObjectAssignData->AddRow(FName(RowFixedName, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(RowData));
    }

	void PalBuildingModLoader::SetupCropData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
	{
        auto TableRow = m_mapObjectFarmCrop->FindRowUnchecked(BuildingId);
        auto TableRowStruct = m_mapObjectFarmCrop->GetRowStruct().Get();
        if (TableRow)
        {
            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());
                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at(PropertyName));
                    }
                }
            }
            catch (const std::exception& e)
            {
                PS::Log<LogLevel::Error>(STR("Failed to modify Row '{}' in {}: {}\n"), BuildingId.ToString(), m_mapObjectFarmCrop->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
        else
        {
            auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
            TableRowStruct->InitializeStruct(RowData);

            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());
                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at(PropertyName));
                    }
                }

                if (Data.contains("CropItemId"))
                {
                    auto CropItemId = Data.at("CropItemId").get<std::string>();
                    m_mapObjectFarmCrop->AddRow(FName(RC::to_generic_string(CropItemId), FNAME_Add), *static_cast<RC::Unreal::FTableRowBase*>(RowData));
                }
            }
            catch (const std::exception& e)
            {
                FMemory::Free(RowData);
                PS::Log<LogLevel::Error>(STR("Failed to add Row '{}' to {}: {}\n"), BuildingId.ToString(), m_mapObjectFarmCrop->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
	}

	void PalBuildingModLoader::SetupItemProductData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
	{
        ImportJson(BuildingId, Data, m_mapObjectItemProductDataTable);
	}

    void PalBuildingModLoader::SetupTechnologyData(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
    {
        auto TableRow = m_technologyRecipeUnlockTable->FindRowUnchecked(BuildingId);
        auto TableRowStruct = m_technologyRecipeUnlockTable->GetRowStruct().Get();
        if (TableRow)
        {
            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());
                    if (PropertyName == "Name" || PropertyName == "Description")
                    {
                        continue;
                    }

                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at(PropertyName));
                    }
                }
            }
            catch (const std::exception& e)
            {
                PS::Log<LogLevel::Error>(STR("Failed to modify Row '{}' in {}: {}\n"), BuildingId.ToString(), m_technologyRecipeUnlockTable->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
        else
        {
            auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
            TableRowStruct->InitializeStruct(RowData);

            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());

                    if (PropertyName == "Name")
                    {
                        auto TechnologyName = std::format(STR("NAME_RECIPE_{}"), BuildingId.ToString());
                        auto TechnologyRowName = FName(TechnologyName, FNAME_Add);
                        FMemory::Memcpy(Property->ContainerPtrToValuePtr<void>(RowData), &TechnologyRowName, sizeof(FName));
                        continue;
                    }

                    if (PropertyName == "Description")
                    {
                        auto TechnologyDescription = std::format(STR("DESC_RECIPE_{}"), BuildingId.ToString());
                        auto TechnologyRowDescription = FName(TechnologyDescription, FNAME_Add);
                        FMemory::Memcpy(Property->ContainerPtrToValuePtr<void>(RowData), &TechnologyRowDescription, sizeof(FName));
                        continue;
                    }

                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at(PropertyName));
                    }
                }

                m_technologyRecipeUnlockTable->AddRow(BuildingId, *static_cast<RC::Unreal::FTableRowBase*>(RowData));
            }
            catch (const std::exception& e)
            {
                FMemory::Free(RowData);
                PS::Log<LogLevel::Error>(STR("Failed to add Row '{}' to {}: {}\n"), BuildingId.ToString(), m_technologyRecipeUnlockTable->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
    }

    void PalBuildingModLoader::SetupTranslations(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data)
    {
        if (Data.contains("Name"))
        {
            auto RowId = std::format(STR("MAPOBJECT_NAME_{}"), BuildingId.ToString());
            SetupTranslation(RowId, m_mapObjectNameTable, Data.at("Name"));
        }

        if (Data.contains("Description"))
        {
            auto RowId = std::format(STR("BUILDOBJECT_DESC_{}"), BuildingId.ToString());
            SetupTranslation(RowId, m_buildObjectDescTable, Data.at("Description"));
        }

        if (Data.contains("Technology"))
        {
            auto Technology = Data.at("Technology");
            if (Technology.is_object())
            {
                if (Technology.contains("Name"))
                {
                    auto RowId = std::format(STR("NAME_RECIPE_{}"), BuildingId.ToString());
                    SetupTranslation(RowId, m_technologyNameTable, Technology.at("Name"));
                }

                if (Technology.contains("Description"))
                {
                    auto RowId = std::format(STR("DESC_RECIPE_{}"), BuildingId.ToString());
                    SetupTranslation(RowId, m_technologyDescTable, Technology.at("Description"));
                }
            }
        }
    }

    void PalBuildingModLoader::SetupTranslation(const RC::StringType& RowKey, RC::Unreal::UDataTable* DataTable, const nlohmann::json& Value)
    {
        auto TranslationRowStruct = DataTable->GetRowStruct().Get();
        auto TextProperty = TranslationRowStruct->GetPropertyByName(STR("TextData"));
        if (TextProperty)
        {
            auto RowKeyName = FName(RowKey, FNAME_Add);

            auto ExistingRow = DataTable->FindRowUnchecked(RowKeyName);
            if (ExistingRow)
            {
                PropertyHelper::CopyJsonValueToContainer(ExistingRow, TextProperty, Value);
            }
            else
            {
                auto TranslationRowData = FMemory::Malloc(TranslationRowStruct->GetStructureSize());
                TranslationRowStruct->InitializeStruct(TranslationRowData);

                try
                {
                    PropertyHelper::CopyJsonValueToContainer(TranslationRowData, TextProperty, Value);
                    DataTable->AddRow(RowKeyName, *reinterpret_cast<RC::Unreal::FTableRowBase*>(TranslationRowData));
                }
                catch (const std::exception& e)
                {
                    FMemory::Free(TranslationRowData);
                    throw std::runtime_error(e.what());
                }
            }
        }
    }

    void PalBuildingModLoader::ImportJson(const RC::Unreal::FName& BuildingId, const nlohmann::json& Data, RC::Unreal::UDataTable* DataTable)
    {
        auto TableRow = DataTable->FindRowUnchecked(BuildingId);
        auto TableRowStruct = DataTable->GetRowStruct().Get();
        if (TableRow)
        {
            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());
                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(TableRow, Property, Data.at(PropertyName));
                    }
                }
            }
            catch (const std::exception& e)
            {
                PS::Log<LogLevel::Error>(STR("Failed to modify Row '{}' in {}: {}\n"), BuildingId.ToString(), DataTable->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
        else
        {
            auto RowData = FMemory::Malloc(TableRowStruct->GetStructureSize());
            TableRowStruct->InitializeStruct(RowData);
            try
            {
                for (FProperty* Property : TFieldRange<FProperty>(TableRowStruct, EFieldIterationFlags::IncludeSuper))
                {
                    auto PropertyName = RC::to_string(Property->GetName());
                    if (Data.contains(PropertyName))
                    {
                        PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at(PropertyName));
                    }
                }

                DataTable->AddRow(BuildingId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(RowData));
            }
            catch (const std::exception& e)
            {
                FMemory::Free(RowData);
                PS::Log<LogLevel::Error>(STR("Failed to add Row '{}' to {}: {}\n"), BuildingId.ToString(), DataTable->GetFullName(), RC::to_generic_string(e.what()));
            }
        }
    }

    RC::StringType PalBuildingModLoader::GetAssignIDSuffixByWorkType(const std::string& WorkType)
    {
        if (WorkType == "EPalWorkType::Seeding" || WorkType == "Seeding")
        {
            return STR("_5");
        }
        else if (WorkType == "EPalWorkType::Watering_Farm" || WorkType == "Watering_Farm")
        {
            return STR("_2");
        }
        else if (WorkType == "EPalWorkType::FarmHarvest" || WorkType == "FarmHarvest")
        {
            return STR("_4");
        }
        else
        {
            return STR("_0");
        }
    }

    int PalBuildingModLoader::GetNextRadialIndex()
    {
        return ++RadialIndex;
    }
}