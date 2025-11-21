#include "Unreal/UClass.hpp"
#include "Unreal/UObject.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "Unreal/Property/FNameProperty.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "SDK/Classes/UDataAsset.h"
#include "SDK/Classes/KismetInternationalizationLibrary.h"
#include "SDK/Structs/Custom/FManagedStruct.h"
#include "SDK/Structs/FPalCharacterIconDataRow.h"
#include "SDK/Structs/FPalBPClassDataRow.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include "Loader/PalSkinModLoader.h"

using namespace RC;
using namespace RC::Unreal;

UClass* PalSkinDataPalCharacterClass = nullptr;

namespace Palworld {
	PalSkinModLoader::PalSkinModLoader() : PalModLoaderBase("skins") {}

	PalSkinModLoader::~PalSkinModLoader() {}

	void PalSkinModLoader::Initialize()
	{
		m_skinDataAsset = UECustom::UObjectGlobals::StaticFindObject<UECustom::UDataAsset*>(nullptr, nullptr,
			STR("/Game/Pal/DataAsset/Skin/DA_StaticSkinDataAsset.DA_StaticSkinDataAsset"));

		m_skinIconTable = UECustom::UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
            STR("/Game/Pal/DataTable/Character/DT_PalCharacterIconDataTable_SkinOverride.DT_PalCharacterIconDataTable_SkinOverride"));

        m_skinTranslationTable = UECustom::UObjectGlobals::StaticFindObject<RC::Unreal::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Text/DT_UI_Common_Text.DT_UI_Common_Text"));

        PS::Log<LogLevel::Verbose>(STR("Initialized SkinModLoader\n"));
	}

	void PalSkinModLoader::Load(const nlohmann::json& json)
	{
        auto StaticSkinMapProperty = PropertyHelper::GetPropertyByName(m_skinDataAsset->GetClassPrivate(), STR("StaticSkinMap"));
		if (!StaticSkinMapProperty)
		{
			throw std::runtime_error("Property 'StaticSkinMap' has changed name in DA_StaticSkinDataAsset, update is required");
		}

        auto StaticSkinMap = StaticSkinMapProperty->ContainerPtrToValuePtr<TMap<FName, UObject*>>(m_skinDataAsset);
		for (auto& [skin_id, value] : json.items())
		{
			auto SkinId = FName(RC::to_generic_string(skin_id), FNAME_Add);
			if (StaticSkinMap->Contains(SkinId))
			{
				auto Row = StaticSkinMap->FindRef(SkinId);
				Edit(SkinId, Row, value);
			}
			else
			{
				Add(SkinId, StaticSkinMap, value);
			}
		}
	}

	void PalSkinModLoader::Add(const FName& SkinId, TMap<FName, UObject*>* StaticSkinMap, const nlohmann::json& Data)
	{
		if (!Data.contains("SkinType"))
		{
			throw std::runtime_error("You must supply SkinType when adding new skins.");
		}

		if (!Data.contains("NormalCharacterClass"))
		{
			throw std::runtime_error("You must supply NormalCharacterClass when adding new skins.");
		}

		if (!Data.contains("BossCharacterClass"))
		{
			throw std::runtime_error("You must supply BossCharacterClass when adding new skins.");
		}

		if (!Data.contains("TargetPalName"))
		{
			throw std::runtime_error("You must supply TargetPalName when adding new skins.");
		}

		auto TargetPalName = FName(RC::to_generic_string(Data.at("TargetPalName").get<std::string>()), FNAME_Add);

		UClass* DatabaseClass = nullptr;

		auto Type = Data.at("SkinType").get<std::string>();
		if (Type == "EPalSkinType::Pal" || Type == "Pal")
		{
			DatabaseClass = LoadOrCacheSkinDataBaseClass(STR("/Script/Pal.PalSkinDataPalCharacterClass"));
		}
		else if (Type == "EPalSkinType::Head" || Type == "Head" || Type == "EPalSkinType::Body" || Type == "Body")
		{
			DatabaseClass = LoadOrCacheSkinDataBaseClass(STR("/Script/Pal.PalSkinDataArmor"));
		}
		else
		{
			throw std::runtime_error(std::format("Unsupported skin type '{}'", Type));
		}

        if (!DatabaseClass)
        {
            throw std::runtime_error(std::format("Failed to set Database Class for '{}'", Type));
        }
        
        auto SkinNameProperty = PropertyHelper::GetPropertyByName<FNameProperty>(DatabaseClass, STR("SkinName"));
		if (!SkinNameProperty)
		{
			throw std::runtime_error("Property 'SkinName' has changed in DA_StaticSkinDataAsset. Update to Pal Schema is needed.");
		}

		FStaticConstructObjectParameters ConstructParams(DatabaseClass, m_skinDataAsset);
		ConstructParams.Name = NAME_None;

		auto Item = UObjectGlobals::StaticConstructObject(ConstructParams);
        SkinNameProperty->SetPropertyValueInContainer(Item, SkinId);

        for (auto& [Key, Value] : Data.items())
        {
            auto KeyWide = RC::to_generic_string(Key);
            auto Property = PropertyHelper::GetPropertyByName(DatabaseClass, KeyWide);
            if (Property)
            {
                PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(Item), Property, Value);
            }
        }

		if (Data.contains("IconTexture"))
		{
			auto IconRowStruct = m_skinIconTable->GetRowStruct().Get();
			auto IconProperty = PropertyHelper::GetPropertyByName(IconRowStruct, STR("Icon"));
			if (!IconProperty)
			{
				throw std::runtime_error("Property 'Icon' has changed in DT_PalCharacterIconDataTable_SkinOverride. Update to Pal Schema is needed.");
			}

            FManagedStruct IconRowData(IconRowStruct);
			try
			{
				PropertyHelper::CopyJsonValueToContainer(IconRowData.GetData(), IconProperty, Data.at("IconTexture"));
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error(e.what());
			}

			m_skinIconTable->AddRow(SkinId, *reinterpret_cast<RC::Unreal::FTableRowBase*>(IconRowData.GetData()));
		}

		if (Data.contains("Name"))
		{
			AddTranslation(SkinId, Data.at("Name"));
		}

		StaticSkinMap->Add(SkinId, Item);

		PS::Log<RC::LogLevel::Normal>(STR("Added new Skin '{}'\n"), SkinId.ToString());
	}

	void PalSkinModLoader::Edit(const FName& SkinId, UObject* Item, const nlohmann::json& Data)
	{
        auto SkinClass = Item->GetClassPrivate();
        if (!SkinClass)
        {
            throw std::runtime_error(std::format("Skin Class for {} was invalid", RC::to_string(SkinId.ToString())));
        }

        for (auto& [Key, Value] : Data.items())
        {
            auto KeyWide = RC::to_generic_string(Key);
            auto Property = PropertyHelper::GetPropertyByName(SkinClass, KeyWide);
            if (Property)
            {
                PropertyHelper::CopyJsonValueToContainer(reinterpret_cast<uint8_t*>(Item), Property, Value);
            }
        }

        auto SkinNameProperty = PropertyHelper::GetPropertyByName<FNameProperty>(SkinClass, STR("SkinName"));
        if (!SkinNameProperty)
        {
            throw std::runtime_error("Property 'SkinName' has changed in DA_StaticSkinDataAsset. Update to Pal Schema is needed.");
        }

        if (Data.contains("Icon"))
        {
            auto IconRowStruct = m_skinIconTable->GetRowStruct().Get();
            auto IconProperty = PropertyHelper::GetPropertyByName(IconRowStruct, STR("Icon"));
            if (IconProperty)
            {
                auto Row = m_skinIconTable->FindRowUnchecked(*SkinNameProperty->GetPropertyValuePtrInContainer(Item));
                if (Row)
                {
                    PropertyHelper::CopyJsonValueToContainer(Row, IconProperty, Data.at("Icon"));
                }
            }
        }

		if (Data.contains("Name"))
		{
			EditTranslation(SkinId, Data.at("Name"));
		}
	}

	void PalSkinModLoader::AddTranslation(const RC::Unreal::FName& SkinId, const nlohmann::json& Data)
	{
		auto FixedSkinId = std::format(STR("SKIN_NAME_{}"), SkinId.ToString());
		auto TranslationRowStruct = m_skinTranslationTable->GetRowStruct().Get();

        auto TextProperty = PropertyHelper::GetPropertyByName(TranslationRowStruct, STR("TextData"));
		if (TextProperty)
		{
            FManagedStruct TranslationRowData(TranslationRowStruct);
			try
			{
				PropertyHelper::CopyJsonValueToContainer(TranslationRowData.GetData(), TextProperty, Data);
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error(e.what());
			}

			m_skinTranslationTable->AddRow(FName(FixedSkinId, FNAME_Add), *reinterpret_cast<RC::Unreal::FTableRowBase*>(TranslationRowData.GetData()));
		}
	}

	void PalSkinModLoader::EditTranslation(const RC::Unreal::FName& SkinId, const nlohmann::json& Data)
	{
		auto FixedSkinId = std::format(STR("SKIN_NAME_{}"), SkinId.ToString());
		auto TranslationRowStruct = m_skinTranslationTable->GetRowStruct().Get();

        auto TextProperty = PropertyHelper::GetPropertyByName(TranslationRowStruct, STR("TextData"));
		if (TextProperty)
		{
			auto Row = m_skinTranslationTable->FindRowUnchecked(FName(FixedSkinId, FNAME_Add));
			if (Row)
			{
				PropertyHelper::CopyJsonValueToContainer(Row, TextProperty, Data);
			}
		}
	}

	RC::Unreal::UClass* PalSkinModLoader::LoadOrCacheSkinDataBaseClass(const RC::StringType& Path)
	{
		auto Iterator = m_palSkinDataBaseClassCache.find(Path);
		if (Iterator != m_palSkinDataBaseClassCache.end())
		{
			return Iterator->second;
		}

		auto NewSkinDataBaseClass = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, Path.c_str());
		if (!NewSkinDataBaseClass)
		{
			throw std::runtime_error(std::format("Failed to cache PalSkinDataBaseClass, '{}' doesn't exist", RC::to_string(Path)));
		}

		return NewSkinDataBaseClass;
	}
}