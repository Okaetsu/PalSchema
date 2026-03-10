#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "Unreal/NameTypes.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/UCompositeDataTable.h"
#include "SDK/Classes/Custom/UDataTableStore.h"
#include "SDK/Structs/Custom/FManagedStruct.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Utility/JsonHelpers.h"
#include "Loader/PalRawTableLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace Palworld {
	PalRawTableLoader::PalRawTableLoader() : PalModLoaderBase("raw") {
        SetDisplayName(TEXT("Raw Table Loader"));
    }

	PalRawTableLoader::~PalRawTableLoader() {}

    void PalRawTableLoader::Apply(const RC::StringType& tableName, RC::Unreal::UDataTable* datatable)
    {
        auto it = m_tableDataMap.find(tableName);
        if (it != m_tableDataMap.end())
        {
            LoadResult result{};

            for (auto& data : it->second)
            {
                Apply(data, datatable, result);
            }

            PS::Log<LogLevel::Normal>(STR("{}: {} rows updated, {} rows added, {} rows deleted, {} error{}.\n"),
                datatable->GetName(), result.SuccessfulModifications, result.SuccessfulAdditions,
                result.SuccessfulDeletions, result.ErrorCount, result.ErrorCount > 1 || result.ErrorCount == 0 ? STR("s") : STR(""));
        }
    }

    void PalRawTableLoader::Apply(UECustom::UCompositeDataTable* compositeDatatable)
    {
        auto parentTables = compositeDatatable->GetParentTables();
        for (auto& parentTable : parentTables)
        {
            auto parentTableName = parentTable->GetName();
            if (parentTableName.ends_with(STR("_Common")))
            {
                Apply(compositeDatatable->GetName(), parentTable.Get());
            }
        }
    }

    void PalRawTableLoader::Apply(const nlohmann::json& data, RC::Unreal::UDataTable* datatable, LoadResult& outResult)
    {
        for (auto& [dataKey, dataRow] : data.items())
        {
            if (dataKey == "Rows")
            {
                outResult.ErrorCount++;
                PS::Log<LogLevel::Error>(STR("When copying entries from FModel, make sure to not include the 'Rows' field and instead add your row entries directly. See https://okaetsu.github.io/PalSchema/docs/guides/rawtables/intro for more info.\n"));
                continue;
            }

            if (dataKey.contains("*"))
            {
                HandleFilters(datatable, dataRow, outResult);
                continue;
            }

            auto rowKeyName = FName(RC::to_generic_string(dataKey), FNAME_Add);
            if (dataRow.is_null())
            {
                DeleteRow(datatable, rowKeyName, outResult);
                continue;
            }

            auto row = datatable->FindRowUnchecked(rowKeyName);
            if (!row)
            {
                AddRow(datatable, rowKeyName, dataRow, outResult);
                continue;
            }

            EditRow(datatable, rowKeyName, row, dataRow, outResult);
        }
    }

    void PalRawTableLoader::OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase != EEngineLifecyclePhase::PostEngineInit)
        {
            return;
        }

        PS::JsonHelpers::ParseJsonFilesInPath(loaderPath, [&](const nlohmann::json& data) {
            for (auto& [Key, Value] : data.items())
            {
                AddToTableDataMap(Key, Value);
            }
        });
    }

    void PalRawTableLoader::OnAutoReload(const RC::StringType& modName, const nlohmann::json& data)
    {
        for (auto& [key, value] : data.items())
        {
            auto datatable = GetDatatableByName(key);
            if (!datatable)
            {
                PS::Log<LogLevel::Error>(STR("Failed to auto-reload {}, data table {} doesn't exist.\n"), modName, RC::to_generic_string(key));
                return;
            }

            auto name = datatable->GetNamePrivate().ToString();
            LoadResult result;
            Apply(value, datatable, result);

            PS::Log<LogLevel::Normal>(STR("{}: {} rows updated, {} rows added, {} rows deleted, {} error{}.\n"),
                name, result.SuccessfulModifications, result.SuccessfulAdditions,
                result.SuccessfulDeletions, result.ErrorCount, result.ErrorCount > 1 || result.ErrorCount == 0 ? STR("s") : STR(""));
        }
    }

    bool PalRawTableLoader::CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase == EEngineLifecyclePhase::PostEngineInit)
        {
            return true;
        }

        return false;
    }

    bool PalRawTableLoader::OnInitialize()
    {
        return true;
    }

    void PalRawTableLoader::OnDatatableSerialized(RC::Unreal::UDataTable* datatable)
    {
        if (!datatable) return;

        if (datatable->GetClassPrivate() == UECustom::UCompositeDataTable::StaticClass())
        {
            auto compositeDatatable = static_cast<UECustom::UCompositeDataTable*>(datatable);
            Apply(compositeDatatable);
        }
        else
        {
            Apply(datatable->GetName(), datatable);
        }
    }

    void PalRawTableLoader::HandleFilters(RC::Unreal::UDataTable* datatable, const nlohmann::json& data, LoadResult& outResult)
    {
        try
        {
            if (data.is_null())
            {
                outResult.SuccessfulDeletions += datatable->GetRowMap().Num();
                datatable->EmptyTable();
            }
            else
            {
                for (auto& [key, row] : datatable->GetRowMap())
                {
                    ModifyRowProperties(datatable, key, row, data, outResult);
                    outResult.SuccessfulModifications++;
                }
            }
        }
        catch (const std::exception& e)
        {
            outResult.ErrorCount++;
            PS::Log<LogLevel::Error>(STR("Failed to do wildcard modification in {}: {}\n"), datatable->GetNamePrivate().ToString(), RC::to_generic_string(e.what()));
        }
    }

    void PalRawTableLoader::AddRow(RC::Unreal::UDataTable* datatable, const FName& rowName, const nlohmann::json& data, LoadResult& outResult)
    {
        auto rowStruct = datatable->GetRowStruct().Get();
        FManagedStruct newRowData(rowStruct);

        try
        {
            ModifyRowProperties(datatable, rowName, newRowData.GetData(), data, outResult);
            datatable->AddRow(rowName, *reinterpret_cast<RC::Unreal::FTableRowBase*>(newRowData.GetData()));
            outResult.SuccessfulAdditions++;
        }
        catch (const std::exception& e)
        {
            auto tableName = datatable->GetNamePrivate().ToString();
            outResult.ErrorCount++;
            PS::Log<LogLevel::Error>(STR("Failed to add Row '{}' in {}: {}\n"), rowName.ToString(), tableName, RC::to_generic_string(e.what()));
        }
    }

    void PalRawTableLoader::EditRow(RC::Unreal::UDataTable* datatable, const FName& rowName, uint8* row, const nlohmann::json& data, LoadResult& outResult)
    {
        try
        {
            ModifyRowProperties(datatable, rowName, row, data, outResult);
            outResult.SuccessfulModifications++;
        }
        catch (const std::exception& e)
        {
            auto tableName = datatable->GetNamePrivate().ToString();
            outResult.ErrorCount++;
            PS::Log<LogLevel::Error>(STR("Failed to edit Row '{}' in {}: {}\n"), rowName.ToString(), tableName, RC::to_generic_string(e.what()));
        }
    }

    void PalRawTableLoader::DeleteRow(RC::Unreal::UDataTable* datatable, const RC::Unreal::FName& rowName, LoadResult& outResult)
    {
        datatable->RemoveRow(rowName);
        outResult.SuccessfulDeletions++;
    }

    void PalRawTableLoader::ModifyRowProperties(RC::Unreal::UDataTable* datatable, const FName& rowName, void* rowPtr, const nlohmann::json& data,
                                                LoadResult& outResult)
    {
        if (!data.is_object())
        {
            throw std::runtime_error(std::format("Value for {} must be an object", RC::to_string(rowName.ToString())));
        }

        auto rowStruct = datatable->GetRowStruct().Get();
        for (auto& [key, value] : data.items())
        {
            auto keyWide = RC::to_generic_string(key);
            auto property = PropertyHelper::GetPropertyByName(rowStruct, keyWide);
            if (property)
            {
                PropertyHelper::CopyJsonValueToContainer(rowPtr, property, value);
            }
            else
            {
                outResult.ErrorCount++;
                PS::Log<LogLevel::Warning>(STR("Property '{}' not found in Row '{}' in {}.\n"), keyWide, rowName.ToString(), datatable->GetNamePrivate().ToString());
            }
        }
    }

    void PalRawTableLoader::AddToTableDataMap(const std::string& datatableName, const nlohmann::json& data)
    {
        auto datatableNameWide = RC::to_generic_string(datatableName);
        auto it = m_tableDataMap.find(datatableNameWide);
        if (it != m_tableDataMap.end())
        {
            it->second.push_back(data);
        }
        else
        {
            std::vector<nlohmann::json> newDataArray{
                data
            };
            m_tableDataMap.emplace(datatableNameWide, newDataArray);
        }
    }
}