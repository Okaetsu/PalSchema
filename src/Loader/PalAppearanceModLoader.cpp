#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/SoftObjectPtr.hpp"
#include "SDK/Classes/KismetInternationalizationLibrary.h"
#include "SDK/Structs/Custom/FManagedStruct.h"
#include "SDK/Structs/FLinearColor.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Utility/JsonHelpers.h"
#include "Helpers/String.hpp"
#include "Loader/PalAppearanceModLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalAppearanceModLoader::PalAppearanceModLoader() : PalModLoaderBase("appearance")
	{
        SetDisplayName(TEXT("Appearance Mod Loader"));
	}

	PalAppearanceModLoader::~PalAppearanceModLoader()
	{
	}

    void PalAppearanceModLoader::OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase != EEngineLifecyclePhase::GameInstanceInit)
        {
            return;
        }

        PS::JsonHelpers::ParseJsonFilesInPath(loaderPath, [&](const nlohmann::json& data) {
            LoadAppearances(data);
        });
    }

    void PalAppearanceModLoader::OnAutoReload(const std::filesystem::path::string_type& modName, const std::filesystem::path& modFilePath)
    {
        PS::JsonHelpers::ParseJsonFilesInPath(modFilePath, [&](const nlohmann::json& data) {
            LoadAppearances(data);
        });
    }

    bool PalAppearanceModLoader::CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase == EEngineLifecyclePhase::GameInstanceInit)
        {
            return true;
        }

        return false;
    }

    bool PalAppearanceModLoader::OnInitialize()
    {
        try
        {
            m_hairTable = GetDatatableByName("DT_CharacterCreationMeshPresetTable_Hair");
            m_headTable = GetDatatableByName("DT_CharacterCreationMeshPresetTable_Head");
            m_eyesTable = GetDatatableByName("DT_CharacterCreationEyeMaterialPresetTable");
            m_bodyTable = GetDatatableByName("DT_CharacterCreationMeshPresetTable_Body");
            m_presetTable = GetDatatableByName("DT_CharacterCreationMakeInfoPreset");
            m_colorPresetTable = GetDatatableByName("DT_CharacterCreationColorPresetTable");
            m_equipmentTable = GetDatatableByName("DT_CharacterCreationMeshPresetTable_Equipments");
        }
        catch (const std::exception& e)
        {
            PS::Log<LogLevel::Error>(STR("Unable to initialize {}, {}\n"), GetDisplayName(), RC::to_generic_string(e.what()));
            return false;
        }

        return true;
    }

    void PalAppearanceModLoader::LoadAppearances(const nlohmann::json& data)
    {
        for (auto& [Key, Value] : data.items())
        {
            auto RowId = FName(RC::to_generic_string(Key), FNAME_Add);
            if (!Value.contains("Type"))
            {
                throw std::runtime_error(std::format("{} had invalid data, property 'Type' must be set to either Hair, Head, Eyes, CharacterPreset or Equipment", Key));
            }

            if (!Value.at("Type").is_string())
            {
                throw std::runtime_error(std::format("{} had invalid data, property 'Type' must be a string", Key));
            }

            auto Type = Value.at("Type").get<std::string>();
            if (Type == "Hair")
            {
                Add(RowId, m_hairTable, Value, { "SkeletalMesh", "IconTexture", "ABPAsset" });
            }
            else if (Type == "Head")
            {
                Add(RowId, m_headTable, Value, { "SkeletalMesh", "SkeletalMesh_MaleHead", "IconTexture" });
            }
            else if (Type == "Eyes")
            {
                Add(RowId, m_eyesTable, Value, { "EyeMaterialInstance", "IconTexture" });
            }
            else if (Type == "Body")
            {
                Add(RowId, m_bodyTable, Value, { "SkeletalMesh", "IconTexture" });
            }
            else if (Type == "CharacterPreset")
            {
                AddPreset(RowId, Value);
            }
            else if (Type == "ColorPreset")
            {
                AddColorPreset(RowId, Value);
            }
            else if (Type == "Equipment")
            {
                AddEquipment(RowId, Value);
            }
            else
            {
                throw std::runtime_error(std::format("Unsupported Type '{}' in {}", Type, Key));
            }

            PS::Log<RC::LogLevel::Normal>(STR("Added new appearance '{}' as '{}'\n"), RowId.ToString(), RC::to_generic_string(Type));
        }
    }

	void PalAppearanceModLoader::Add(const RC::Unreal::FName& RowId, RC::Unreal::UDataTable* DataTable, const nlohmann::json& Data, const std::vector<std::string>& RequiredFields)
	{
		auto RowStruct = DataTable->GetRowStruct().Get();

		for (auto& RequiredField : RequiredFields)
		{
			if (!RowStruct->GetPropertyByName(RC::to_generic_string(RequiredField).c_str()))
			{
				throw std::runtime_error(std::format("Property {} has changed name in {}, update is required", RequiredField, RC::to_string(RowStruct->GetName())));
			}
			if (!Data.contains(RequiredField))
			{
				throw std::runtime_error(std::format("Missing required field {} in {}", RequiredField, RC::to_string(RowId.ToString())));
			}
		}

        FManagedStruct NewRow(RowStruct);

		for (FProperty* Property : TFieldRange<FProperty>(RowStruct, EFieldIterationFlags::Default))
		{
			auto PropertyName = RC::to_string(Property->GetName());
			if (Data.contains(PropertyName))
			{
				try
				{
					PropertyHelper::CopyJsonValueToContainer(NewRow.GetData(), Property, Data.at(PropertyName));
				}
				catch (const std::exception& e)
				{
					throw std::runtime_error(std::format("{} in {}", e.what(), RC::to_string(RowId.ToString())));
				}
			}
		}

		DataTable->AddRow(RowId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(NewRow.GetData()));
	}

	void PalAppearanceModLoader::AddPreset(const RC::Unreal::FName& PresetId, const nlohmann::json& Data)
	{
		auto RowStruct = m_presetTable->GetRowStruct().Get();

		if (!RowStruct->GetPropertyByName(TEXT("IconTexture")))
		{
			throw std::runtime_error(std::format("Property IconTexture has changed name in {}, update is required", RC::to_string(RowStruct->GetName())));
		}
		if (!Data.contains("IconTexture"))
		{
			throw std::runtime_error(std::format("Missing required field IconTexture in {}", RC::to_string(PresetId.ToString())));
		}
		if (!Data.at("IconTexture").is_string())
		{
			throw std::runtime_error(std::format("IconTexture field in {} must be a string", RC::to_string(PresetId.ToString())));
		}

        FStructProperty* MakeInfoProperty = static_cast<FStructProperty*>(RowStruct->GetPropertyByName(TEXT("MakeInfo")));
		if (!MakeInfoProperty)
		{
			throw std::runtime_error(std::format("Property MakeInfo has changed name in {}, update is required", RC::to_string(RowStruct->GetName())));
		}
		if (!Data.contains("MakeInfo"))
		{
			throw std::runtime_error(std::format("Missing required field MakeInfo in {}", RC::to_string(PresetId.ToString())));
		}
        auto& MakeInfoData = Data.at("MakeInfo");
		if (!MakeInfoData.is_object())
		{
			throw std::runtime_error(std::format("MakeInfo field in {} must be an object", RC::to_string(PresetId.ToString())));
		}

        UScriptStruct* MakeInfoStruct = MakeInfoProperty->GetStruct().Get();

        const std::vector<std::string> RequiredSubFields = 
            { "BodyMeshName", "HeadMeshName", "HairMeshName", "EquipmentBodyMeshName", "EquipmentHeadMeshName", "EyeMaterialName" };

		for (auto& RequiredSubField : RequiredSubFields)
		{
			if (!MakeInfoStruct->GetPropertyByName(RC::to_generic_string(RequiredSubField).c_str()))
            {
                throw std::runtime_error(std::format("Property {} has changed name in MakeInfo, update is required", RequiredSubField));
            }
            if (!MakeInfoData.contains(RequiredSubField))
            {
                throw std::runtime_error(std::format("Missing required field {} in MakeInfo", RequiredSubField));
			}
		}
		
        FManagedStruct NewRow(RowStruct);

		for (FProperty* Property : TFieldRange<FProperty>(RowStruct, EFieldIterationFlags::Default))
		{
			auto PropertyName = RC::to_string(Property->GetName());
			if (Data.contains(PropertyName))
			{
				try
				{
					PropertyHelper::CopyJsonValueToContainer(NewRow.GetData(), Property, Data.at(PropertyName));
				}
				catch (const std::exception& e)
				{
					throw std::runtime_error(std::format("{} in {}", e.what(), RC::to_string(PresetId.ToString())));
				}
			}
		}

		m_presetTable->AddRow(PresetId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(NewRow.GetData()));
	}

	void PalAppearanceModLoader::AddColorPreset(const RC::Unreal::FName& ColorPresetId, const nlohmann::json& Data)
	{
		auto ColorPresetRowStruct = m_colorPresetTable->GetRowStruct().Get();

		auto Id = ColorPresetId.ToString();
		if (Id != STR("Skin") && Id != STR("Hair_Brow") && Id != STR("Eye"))
		{
			throw std::runtime_error("Color Preset must be either Skin, Hair_Brow or Eye");
		}

		auto ColorsArrayProperty = ColorPresetRowStruct->GetPropertyByName(STR("Colors"));
		if (!ColorsArrayProperty)
		{
			throw std::runtime_error("Property 'Colors' has changed name in DT_CharacterCreationColorPresetTable, update is required");
		}

		auto ColorRow = m_colorPresetTable->FindRowUnchecked(ColorPresetId);
		if (!ColorRow)
		{
			throw std::runtime_error(std::format("Unable to find row {} in DT_CharacterCreationColorPresetTable", RC::to_string(Id)));
		}

		void* ColorsArrayContainer = ColorsArrayProperty->ContainerPtrToValuePtr<void>(ColorRow);
		TArray<UECustom::FLinearColor>& ColorsArray = *reinterpret_cast<TArray<UECustom::FLinearColor>*>(ColorsArrayContainer);

		if (!Data.contains("Colors"))
		{
			throw std::runtime_error(std::format("{} must have a Colors field", RC::to_string(Id)));
		}

		if (!Data.at("Colors").is_object())
		{
			throw std::runtime_error(std::format("Colors field in {} must be an object", RC::to_string(Id)));
		}

		PropertyHelper::CopyJsonValueToContainer(ColorRow, ColorsArrayProperty, Data.at("Colors"));
	}

	void PalAppearanceModLoader::AddEquipment(const RC::Unreal::FName& EquipmentId, const nlohmann::json& Data)
	{
		if (!Data.contains("SkeletalMeshMap"))
		{
			throw std::runtime_error("You must supply SkeletalMeshMap when adding new equipment");
		}

		if (!Data.contains("ABPAssetMap"))
		{
			throw std::runtime_error("You must supply ABPAssetMap when adding new equipment");
		}

		if (!Data.at("SkeletalMeshMap").is_array())
		{
			throw std::runtime_error("Property SkeletalMeshMap must be an array");
		}

		if (!Data.at("ABPAssetMap").is_array())
		{
			throw std::runtime_error("Property ABPAssetMap must be an array");
		}

		if (Data.contains("IsHairAttachAccessory"))
		{
			if (!Data.at("IsHairAttachAccessory").is_boolean())
			{
				throw std::runtime_error("Property IsHairAttachAccessory must be a boolean");
			}

			if (Data.at("IsHairAttachAccessory").get<bool>() == true)
			{
				if (!Data.contains("HairAttachSocketNameMap"))
				{
					throw std::runtime_error("Property HairAttachSocketNameMap must exist when IsHairAttachAccessory is set to true");
				}

				if (!Data.at("HairAttachSocketNameMap").is_array())
				{
					throw std::runtime_error("Property HairAttachSocketNameMap must be an array");
				}
			}
		}

		auto EquipmentRowStruct = m_equipmentTable->GetRowStruct().Get();

		auto SkeletalMeshMapProperty = Palworld::PropertyHelper::CastProperty<FMapProperty>(EquipmentRowStruct->GetPropertyByName(STR("SkeletalMeshMap")));
		if (!SkeletalMeshMapProperty)
		{
			throw std::runtime_error("Property SkeletalMeshMap has changed name in DT_CharacterCreationMeshPresetTable_Equipments, update is required");
		}

		auto ABPAssetMapProperty = Palworld::PropertyHelper::CastProperty<FMapProperty>(EquipmentRowStruct->GetPropertyByName(STR("ABPAssetMap")));
		if (!ABPAssetMapProperty)
		{
			throw std::runtime_error("Property ABPAssetMap has changed name in DT_CharacterCreationMeshPresetTable_Equipments, update is required");
		}

        FManagedStruct NewRow(EquipmentRowStruct);

		// Skeletal Mesh Map

		auto& JsonSkeletalMeshMap = Data.at("SkeletalMeshMap");
        Palworld::PropertyHelper::CopyJsonValueToContainer(NewRow.GetData(), SkeletalMeshMapProperty, JsonSkeletalMeshMap);

		// ABP Asset Map

		auto& JsonABPAssetMap = Data.at("ABPAssetMap");
        Palworld::PropertyHelper::CopyJsonValueToContainer(NewRow.GetData(), ABPAssetMapProperty, JsonABPAssetMap);

		// Hair Attach Socket Name Map

		if (Data.contains("HairAttachSocketNameMap"))
		{
			auto IsHairAttachAccessoryProperty = EquipmentRowStruct->GetPropertyByName(STR("IsHairAttachAccessory"));
			if (!IsHairAttachAccessoryProperty)
			{
				throw std::runtime_error("Property IsHairAttachAccessory has changed name in DT_CharacterCreationMeshPresetTable_Equipments, update is required");
			}

			PropertyHelper::CopyJsonValueToContainer(NewRow.GetData(), IsHairAttachAccessoryProperty, Data.at("IsHairAttachAccessory"));

			if (Data.at("IsHairAttachAccessory").get<bool>() == true)
			{
				auto HairAttachSocketNameMapProperty = EquipmentRowStruct->GetPropertyByName(STR("HairAttachSocketNameMap"));
				if (!HairAttachSocketNameMapProperty)
				{
					throw std::runtime_error("Property HairAttachSocketNameMap has changed name in DT_CharacterCreationMeshPresetTable_Equipments, update is required");
				}

				void* HairAttachSocketNameMapContainer = HairAttachSocketNameMapProperty->ContainerPtrToValuePtr<void>(NewRow.GetData());
				TMap<FName, FName>& HairAttachSocketNameMap = *reinterpret_cast<TMap<FName, FName>*>(HairAttachSocketNameMapContainer);

				auto JsonHairAttachSocketNameMap = Data.at("HairAttachSocketNameMap").get<std::vector<nlohmann::json>>();
				for (auto& HairAttachSocketName : JsonHairAttachSocketNameMap)
				{
					if (!HairAttachSocketName.contains("Key") || !HairAttachSocketName.contains("Value"))
					{
						throw std::runtime_error("Entry in ABPAssetMap was missing a Key or Value");
					}
					if (!HairAttachSocketName.at("Key").is_string() || !HairAttachSocketName.at("Value").is_string())
					{
						throw std::runtime_error("ABPAssetMap entry Key and Value must be a string");
					}

					auto Key = RC::to_generic_string(HairAttachSocketName.at("Key").get<std::string>());
					auto Value = RC::to_generic_string(HairAttachSocketName.at("Value").get<std::string>());

					HairAttachSocketNameMap.Add(FName(Key, FNAME_Add), FName(Value, FNAME_Add));
				}
			}
		}

        m_equipmentTable->AddRow(EquipmentId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(NewRow.GetData()));
	}
}