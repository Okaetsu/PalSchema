#include "Unreal/UClass.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "Unreal/NameTypes.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "SDK/Classes/UCompositeDataTable.h"
#include "SDK/Structs/Custom/FManagedStruct.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Loader/PalRawTableLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace Palworld {
	PalRawTableLoader::PalRawTableLoader() : PalModLoaderBase("raw") {}

	PalRawTableLoader::~PalRawTableLoader() {}

	void PalRawTableLoader::Initialize() {}

    void PalRawTableLoader::OnDataTableChanged(RC::Unreal::UDataTable* Table)
    {
        if (!Table) return;

        if (Table->GetClassPrivate() == UECustom::UCompositeDataTable::StaticClass())
        {
            auto CompositeTable = static_cast<UECustom::UCompositeDataTable*>(Table);
            Apply(CompositeTable);
        }
        else
        {
            Apply(Table->GetName(), Table);
        }
    }

    void PalRawTableLoader::Apply(const RC::StringType& TableName, RC::Unreal::UDataTable* Table)
    {
        auto It = m_tableDataMap.find(TableName);
        if (It != m_tableDataMap.end())
        {
            LoadResult Result{};

            for (auto& Data : It->second)
            {
                Apply(Data, Table, Result);
            }

            PS::Log<LogLevel::Normal>(STR("{}: {} rows updated, {} rows added, {} rows deleted, {} error{}.\n"),
                Table->GetName(), Result.SuccessfulModifications, Result.SuccessfulAdditions,
                Result.SuccessfulDeletions, Result.ErrorCount, Result.ErrorCount > 1 || Result.ErrorCount == 0 ? STR("s") : STR(""));
        }
    }

    void PalRawTableLoader::Apply(UECustom::UCompositeDataTable* Table)
    {
        auto CompositeTable = static_cast<UECustom::UCompositeDataTable*>(Table);
        auto ParentTables = CompositeTable->GetParentTables();
        for (auto& ParentTable : ParentTables)
        {
            auto ParentTableName = ParentTable->GetName();
            if (ParentTableName.ends_with(STR("_Common")))
            {
                Apply(Table->GetName(), ParentTable.Get());
            }
        }
    }

    void PalRawTableLoader::Apply(const nlohmann::json& Data, RC::Unreal::UDataTable* Table, LoadResult& OutResult)
    {
        auto Name = Table->GetNamePrivate().ToString();
        for (auto& [RowKey, RowData] : Data.items())
        {
            if (RowKey == "Rows")
            {
                OutResult.ErrorCount++;
                PS::Log<LogLevel::Error>(STR("When copying entries from FModel, make sure to not include the 'Rows' field and instead add your row entries directly. See https://okaetsu.github.io/PalSchema/docs/guides/rawtables/intro for more info.\n"));
                continue;
            }

            if (RowKey.contains("*"))
            {
                HandleFilters(Table, RowData, OutResult);
                continue;
            }

            auto RowKeyName = FName(RC::to_generic_string(RowKey), FNAME_Add);
            auto Row = Table->FindRowUnchecked(RowKeyName);
            if (!Row)
            {
                AddRow(Table, RowKeyName, RowData, OutResult);
                continue;
            }

            if (RowData.is_null())
            {
                DeleteRow(Table, RowKeyName, OutResult);
            }
            else
            {
                EditRow(Table, RowKeyName, Row, RowData, OutResult);
            }
        }
    }

	void PalRawTableLoader::Load(const nlohmann::json& Data)
	{
        for (auto& [Key, Value] : Data.items())
        {
            AddToTableDataMap(Key, Value);
        }
	}

    void PalRawTableLoader::Reload(const nlohmann::json& Data)
    {
        for (auto& [Key, Value] : Data.items())
        {
            auto KeyWide = RC::to_generic_string(Key);
            auto TableIt = m_tableMap.find(KeyWide);
            if (TableIt != m_tableMap.end())
            {
                auto Table = TableIt->second;
                auto Name = Table->GetNamePrivate().ToString();
                LoadResult Result;
                Apply(Value, Table, Result);

                PS::Log<LogLevel::Normal>(STR("{}: {} rows updated, {} rows added, {} rows deleted, {} error{}.\n"),
                    Name, Result.SuccessfulModifications, Result.SuccessfulAdditions,
                    Result.SuccessfulDeletions, Result.ErrorCount, Result.ErrorCount > 1 || Result.ErrorCount == 0 ? STR("s") : STR(""));
            }
        }
    }

    void PalRawTableLoader::HandleFilters(RC::Unreal::UDataTable* Table, const nlohmann::json& Data, LoadResult& OutResult)
    {
        try
        {
            auto RowStruct = Table->GetRowStruct().Get();
            for (auto& [Key, Row] : Table->GetRowMap())
            {
                ModifyRowProperties(Table, Key, Row, Data, OutResult);
                OutResult.SuccessfulModifications++;
            }
        }
        catch (const std::exception& e)
        {
            OutResult.ErrorCount++;
            PS::Log<LogLevel::Error>(STR("Failed to do wildcard modification in {}: {}\n"), Table->GetNamePrivate().ToString(), RC::to_generic_string(e.what()));
        }
    }

    void PalRawTableLoader::AddRow(RC::Unreal::UDataTable* Table, const FName& RowName, const nlohmann::json& Data, LoadResult& OutResult)
    {
        auto RowStruct = Table->GetRowStruct().Get();
        FManagedStruct NewRowData(RowStruct);

        try
        {
            ModifyRowProperties(Table, RowName, NewRowData.GetData(), Data, OutResult);
            Table->AddRow(RowName, *reinterpret_cast<RC::Unreal::FTableRowBase*>(NewRowData.GetData()));
            OutResult.SuccessfulAdditions++;
        }
        catch (const std::exception& e)
        {
            auto TableName = Table->GetNamePrivate().ToString();
            OutResult.ErrorCount++;
            PS::Log<LogLevel::Error>(STR("Failed to add Row '{}' in {}: {}\n"), RowName.ToString(), TableName, RC::to_generic_string(e.what()));
        }
    }

    void PalRawTableLoader::EditRow(RC::Unreal::UDataTable* Table, const FName& RowName, uint8* Row, const nlohmann::json& Data, LoadResult& OutResult)
    {
        try
        {
            auto RowStruct = Table->GetRowStruct().Get();
            ModifyRowProperties(Table, RowName, Row, Data, OutResult);
            OutResult.SuccessfulModifications++;
        }
        catch (const std::exception& e)
        {
            auto TableName = Table->GetNamePrivate().ToString();
            OutResult.ErrorCount++;
            PS::Log<LogLevel::Error>(STR("Failed to edit Row '{}' in {}: {}\n"), RowName.ToString(), TableName, RC::to_generic_string(e.what()));
        }
    }

    void PalRawTableLoader::DeleteRow(RC::Unreal::UDataTable* Table, const RC::Unreal::FName& RowName, LoadResult& OutResult)
    {
        Table->RemoveRow(RowName);
        OutResult.SuccessfulDeletions++;
    }

    void PalRawTableLoader::ModifyRowProperties(RC::Unreal::UDataTable* Table, const FName& RowName, void* RowPtr, const nlohmann::json& Data,
                                                LoadResult& OutResult)
    {
        if (!Data.is_object())
        {
            throw std::runtime_error(std::format("Value for {} must be an object", RC::to_string(RowName.ToString())));
        }

        auto RowStruct = Table->GetRowStruct().Get();
        for (auto& [Key, Value] : Data.items())
        {
            auto KeyWide = RC::to_generic_string(Key);
            auto Property = PropertyHelper::GetPropertyByName(RowStruct, KeyWide);
            if (Property)
            {
                PropertyHelper::CopyJsonValueToContainer(RowPtr, Property, Value);
            }
            else
            {
                OutResult.ErrorCount++;
                PS::Log<LogLevel::Warning>(STR("Property '{}' not found in Row '{}' in {}.\n"), KeyWide, RowName.ToString(), Table->GetNamePrivate().ToString());
            }
        }
    }

    void PalRawTableLoader::AddToTableDataMap(const std::string& TableName, const nlohmann::json& Data)
    {
        auto TableNameWide = RC::to_generic_string(TableName);
        auto It = m_tableDataMap.find(TableNameWide);
        if (It != m_tableDataMap.end())
        {
            It->second.push_back(Data);
        }
        else
        {
            std::vector<nlohmann::json> NewDataArray{
                Data
            };
            m_tableDataMap.emplace(TableNameWide, NewDataArray);
        }
    }
}