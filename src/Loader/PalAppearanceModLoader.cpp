#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/KismetInternationalizationLibrary.h"
#include "SDK/Classes/TSoftObjectPtr.h"
#include "SDK/Classes/TSoftClassPtr.h"
#include "SDK/Structs/FLinearColor.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include "Loader/PalAppearanceModLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalAppearanceModLoader::PalAppearanceModLoader() : PalModLoaderBase("appearance")
	{
	}

	PalAppearanceModLoader::~PalAppearanceModLoader()
	{
	}

	void PalAppearanceModLoader::Initialize()
	{
		m_hairTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/CharacteCreation/DT_CharacterCreationMeshPresetTable_Hair.DT_CharacterCreationMeshPresetTable_Hair"));

		m_headTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/CharacteCreation/DT_CharacterCreationMeshPresetTable_Head.DT_CharacterCreationMeshPresetTable_Head"));

		m_eyesTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/CharacteCreation/DT_CharacterCreationEyeMaterialPresetTable.DT_CharacterCreationEyeMaterialPresetTable"));

		m_bodyTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/CharacteCreation/DT_CharacterCreationMeshPresetTable_Body.DT_CharacterCreationMeshPresetTable_Body"));

		m_presetTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/CharacteCreation/DT_CharacterCreationMakeInfoPreset.DT_CharacterCreationMakeInfoPreset"));

		m_colorPresetTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/CharacteCreation/DT_CharacterCreationColorPresetTable.DT_CharacterCreationColorPresetTable"));

		m_equipmentTable = UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/CharacteCreation/DT_CharacterCreationMeshPresetTable_Equipments.DT_CharacterCreationMeshPresetTable_Equipments"));

        PS::Log<LogLevel::Verbose>(STR("Initialized AppearanceModLoader\n"));
	}

	void PalAppearanceModLoader::Load(const nlohmann::json& Data)
	{
		for (auto& [Key, Value] : Data.items())
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
				Add(RowId, m_presetTable, Value, { "BodyMeshName", "HeadMeshName", "EquipmentBodyMeshName", "EquipmentHeadMeshName", "EyeMaterialName" });
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
				throw std::runtime_error(std::format("Property {} has changed name in {}, update is required", RC::to_string(RowStruct->GetName()), RequiredField));
			}
			if (!Data.contains(RequiredField))
			{
				throw std::runtime_error(std::format("Missing required field {} in {}", RequiredField, RC::to_string(RowId.ToString())));
			}
		}

		auto RowData = FMemory::Malloc(RowStruct->GetStructureSize());
		RowStruct->InitializeStruct(RowData);

        for (FProperty* Property : TFieldRange<FProperty>(RowStruct, EFieldIterationFlags::Default))
		{
			auto PropertyName = RC::to_string(Property->GetName());
			if (Data.contains(PropertyName))
			{
				try
				{
					PropertyHelper::CopyJsonValueToContainer(RowData, Property, Data.at(PropertyName));
				}
				catch (const std::exception& e)
				{
					FMemory::Free(RowData);
					throw std::runtime_error(std::format("{} in {}", e.what(), RC::to_string(RowId.ToString())));
				}
			}
		}

		DataTable->AddRow(RowId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(RowData));
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

		auto SkeletalMeshMapProperty = EquipmentRowStruct->GetPropertyByName(STR("SkeletalMeshMap"));
		if (!SkeletalMeshMapProperty)
		{
			throw std::runtime_error("Property SkeletalMeshMap has changed name in DT_CharacterCreationMeshPresetTable_Equipments, update is required");
		}

		auto ABPAssetMapProperty = EquipmentRowStruct->GetPropertyByName(STR("ABPAssetMap"));
		if (!ABPAssetMapProperty)
		{
			throw std::runtime_error("Property ABPAssetMap has changed name in DT_CharacterCreationMeshPresetTable_Equipments, update is required");
		}

		auto EquipmentRowData = FMemory::Malloc(EquipmentRowStruct->GetStructureSize());
		EquipmentRowStruct->InitializeStruct(EquipmentRowData);

		// Skeletal Mesh Map

		void* SkeletalMeshMapContainer = SkeletalMeshMapProperty->ContainerPtrToValuePtr<void>(EquipmentRowData);
		TMap<FName, UECustom::TSoftObjectPtr<UObject>>& SkeletalMeshMap = *reinterpret_cast<TMap<FName, UECustom::TSoftObjectPtr<UObject>>*>(SkeletalMeshMapContainer);

		auto JsonSkeletalMeshMap = Data.at("SkeletalMeshMap").get<std::vector<nlohmann::json>>();
		for (auto& SkeletalMesh : JsonSkeletalMeshMap)
		{
			if (!SkeletalMesh.contains("Key") || !SkeletalMesh.contains("Value"))
			{
				FMemory::Free(EquipmentRowData);
				throw std::runtime_error("Entry in SkeletalMeshMap was missing a Key or Value");
			}
			if (!SkeletalMesh.at("Key").is_string() || !SkeletalMesh.at("Value").is_string())
			{
				FMemory::Free(EquipmentRowData);
				throw std::runtime_error("SkeletalMeshMap entry Key and Value must be a string");
			}

			auto Key = RC::to_generic_string(SkeletalMesh.at("Key").get<std::string>());
			auto Value = RC::to_generic_string(SkeletalMesh.at("Value").get<std::string>());
			auto MeshPath = UECustom::TSoftObjectPtr<UObject>(UECustom::FSoftObjectPath(Value));

			SkeletalMeshMap.Add(FName(Key, FNAME_Add), MeshPath);
		}

		// ABP Asset Map

		void* ABPAssetMapContainer = ABPAssetMapProperty->ContainerPtrToValuePtr<void>(EquipmentRowData);
		TMap<FName, UECustom::TSoftClassPtr<UClass>>& ABPAssetMap = *reinterpret_cast<TMap<FName, UECustom::TSoftClassPtr<UClass>>*>(ABPAssetMapContainer);

		auto JsonABPAssetMap = Data.at("ABPAssetMap").get<std::vector<nlohmann::json>>();
		for (auto& ABPAsset : JsonABPAssetMap)
		{
			if (!ABPAsset.contains("Key") || !ABPAsset.contains("Value"))
			{
				FMemory::Free(EquipmentRowData);
				throw std::runtime_error("Entry in ABPAssetMap was missing a Key or Value");
			}
			if (!ABPAsset.at("Key").is_string() || !ABPAsset.at("Value").is_string())
			{
				FMemory::Free(EquipmentRowData);
				throw std::runtime_error("ABPAssetMap entry Key and Value must be a string");
			}

			auto Key = RC::to_generic_string(ABPAsset.at("Key").get<std::string>());
			auto Value = RC::to_generic_string(ABPAsset.at("Value").get<std::string>());
			auto ABPPath = UECustom::TSoftClassPtr<UClass>(UECustom::FSoftObjectPath(Value));

			ABPAssetMap.Add(FName(Key, FNAME_Add), ABPPath);
		}

		// Hair Attach Socket Name Map

		if (Data.contains("HairAttachSocketNameMap"))
		{
			auto IsHairAttachAccessoryProperty = EquipmentRowStruct->GetPropertyByName(STR("IsHairAttachAccessory"));
			if (!IsHairAttachAccessoryProperty)
			{
				FMemory::Free(EquipmentRowData);
				throw std::runtime_error("Property IsHairAttachAccessory has changed name in DT_CharacterCreationMeshPresetTable_Equipments, update is required");
			}

			PropertyHelper::CopyJsonValueToContainer(EquipmentRowData, IsHairAttachAccessoryProperty, Data.at("IsHairAttachAccessory"));

			if (Data.at("IsHairAttachAccessory").get<bool>() == true)
			{
				auto HairAttachSocketNameMapProperty = EquipmentRowStruct->GetPropertyByName(STR("HairAttachSocketNameMap"));
				if (!HairAttachSocketNameMapProperty)
				{
					FMemory::Free(EquipmentRowData);
					throw std::runtime_error("Property HairAttachSocketNameMap has changed name in DT_CharacterCreationMeshPresetTable_Equipments, update is required");
				}

				void* HairAttachSocketNameMapContainer = HairAttachSocketNameMapProperty->ContainerPtrToValuePtr<void>(EquipmentRowData);
				TMap<FName, FName>& HairAttachSocketNameMap = *reinterpret_cast<TMap<FName, FName>*>(HairAttachSocketNameMapContainer);

				auto JsonHairAttachSocketNameMap = Data.at("HairAttachSocketNameMap").get<std::vector<nlohmann::json>>();
				for (auto& HairAttachSocketName : JsonHairAttachSocketNameMap)
				{
					if (!HairAttachSocketName.contains("Key") || !HairAttachSocketName.contains("Value"))
					{
						FMemory::Free(EquipmentRowData);
						throw std::runtime_error("Entry in ABPAssetMap was missing a Key or Value");
					}
					if (!HairAttachSocketName.at("Key").is_string() || !HairAttachSocketName.at("Value").is_string())
					{
						FMemory::Free(EquipmentRowData);
						throw std::runtime_error("ABPAssetMap entry Key and Value must be a string");
					}

					auto Key = RC::to_generic_string(HairAttachSocketName.at("Key").get<std::string>());
					auto Value = RC::to_generic_string(HairAttachSocketName.at("Value").get<std::string>());

					HairAttachSocketNameMap.Add(FName(Key, FNAME_Add), FName(Value, FNAME_Add));
				}
			}
		}

		m_equipmentTable->AddRow(EquipmentId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(EquipmentRowData));
	}
}