#include "SDK/Classes/Custom/UDataTableStore.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Utility/Logging.h"
#include "Unreal/UClass.hpp"
#include "Unreal/UObject.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "Helpers/String.hpp"

using namespace RC;
using namespace RC::Unreal;

RC::Unreal::UDataTable* UECustom::UDataTableStore::GetTableByName(const std::string& Name)
{
    auto Table = TableMap.find(Name);
    if (Table != TableMap.end())
    {
        return Table->second;
    }

    return nullptr;
}

void UECustom::UDataTableStore::Store(const std::string& Name, RC::Unreal::UDataTable* Table)
{
    TableMap.insert_or_assign(Name, Table);
}

void UECustom::UDataTableStore::Store(RC::Unreal::UDataTable* Table)
{
    auto Name = Table->GetNamePrivate().ToString();
    UECustom::UDataTableStore::Store(RC::to_string(Name), Table);
}

void UECustom::UDataTableStore::Initialize()
{
    TArray<UObject*> Results;

    auto DataTableClass = RC::Unreal::UDataTable::StaticClass();
    if (!DataTableClass)
    {
        PS::Log<LogLevel::Error>(STR("Unable to initialize UDataTableStore, failed to get /Script/Engine.DataTable\n"));
        return;
    }

    PS::Log<LogLevel::Verbose>(STR("UClass for UDataTable found, fetching UDataTables...\n"));
    UECustom::UObjectGlobals::GetObjectsOfClass(DataTableClass, Results);

    int AddedUDataTables = 0;
    for (auto& Object : Results)
    {
        auto DT = static_cast<RC::Unreal::UDataTable*>(Object);
        auto Name = Object->GetNamePrivate().ToString();
        UECustom::UDataTableStore::Store(RC::to_string(Name), DT);
        AddedUDataTables++;
    }

    PS::Log<LogLevel::Verbose>(STR("Finished mapping {} UDataTables.\n"), AddedUDataTables);
}
