#include "Unreal/UClass.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Structs/FPalLocalizedTextData.h"
#include "SDK/Classes/KismetInternationalizationLibrary.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "SDK/Classes/Custom/UDataTableStore.h"
#include "Loader/PalLanguageModLoader.h"
#include "Utility/Config.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalLanguageModLoader::PalLanguageModLoader() : PalModLoaderBase("translations") {}

	PalLanguageModLoader::~PalLanguageModLoader() {}

	void PalLanguageModLoader::Load(const nlohmann::json& Data)
	{
		for (auto& [TableName, TableData] : Data.items())
		{
			for (auto& [RowId, RowValue] : TableData.items())
			{
				auto Table = UECustom::UDataTableStore::GetTableByName(TableName);
				if (Table)
				{
                    auto RowStruct = Table->GetRowStruct();
                    if (!RowStruct.Get())
                    {
                        throw std::runtime_error("RowStruct was invalid");
                    }

                    static auto NAME_PalLocalizedTextData = FName(TEXT("PalLocalizedTextData"), FNAME_Add);
                    if (RowStruct.Get()->GetNamePrivate() != NAME_PalLocalizedTextData)
                    {
                        throw std::runtime_error("Row provided isn't equivalent to PalLocalizedTextData");
                    }

                    if (!RowValue.is_string())
                    {
                        throw std::runtime_error("String value must be provided for a translation");
                    }

					auto RowName = FName(RC::to_generic_string(RowId), FNAME_Add);
					auto Row = std::bit_cast<FPalLocalizedTextData*>(Table->FindRowUnchecked(RowName));
					if (Row)
					{
						Row->TextData = FText(RC::to_generic_string(RowValue.get<std::string>()));
					}
					else
					{
						FPalLocalizedTextData NewRow{};
						NewRow.TextData = FText(RC::to_generic_string(RowValue.get<std::string>()));
						Table->AddRow(RowName, NewRow);
					}
				}
			}
		}
	}

	void PalLanguageModLoader::Initialize()
	{
        auto languageOverride = PS::PSConfig::GetLanguageOverride();

        if (languageOverride == "")
        {
            PS::Log<LogLevel::Verbose>(STR("Fetching current language from Kismet Internationalization Library...\n"));
            auto language = Palworld::UKismetInternationalizationLibrary::GetCurrentLanguage();
            m_currentLanguage = RC::to_string(*language);
            PS::Log<RC::LogLevel::Normal>(STR("Language override not set, using system language ({}).\n"), language.GetCharArray());
        }
        else
        {
            m_currentLanguage = languageOverride;
            PS::Log<RC::LogLevel::Normal>(STR("Language override set to {}.\n"), RC::to_generic_string(languageOverride));
        }

        PS::Log<LogLevel::Verbose>(STR("Initialized LanguageModLoader\n"));
	}

	const std::string& PalLanguageModLoader::GetCurrentLanguage()
	{
		return m_currentLanguage;
	}
}