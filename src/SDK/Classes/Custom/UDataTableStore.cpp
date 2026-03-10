#include "SDK/Classes/Custom/UDataTableStore.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Utility/Logging.h"
#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Unreal/UObject.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "Helpers/String.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    RC::Unreal::UDataTable* UDataTableRegistry::GetDatatableByName(const std::string& name)
    {
        auto datatable = m_datatableMap.find(name);
        if (datatable != m_datatableMap.end())
        {
            return datatable->second;
        }

        return nullptr;
    }

    DatatableSerializeCallbackId UDataTableRegistry::RegisterDatatableSerializeCallback(const DatatableSerializeCallback& callback)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto nextId = GenerateDatatableSerializeCallbackId();
        m_callbackMap.emplace(nextId, callback);

        return nextId;
    }

    void UDataTableRegistry::UnregisterDatatableSerializeCallback(const DatatableSerializeCallbackId& callbackId)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_callbackMap.erase(callbackId);
    }

    void UDataTableRegistry::Add(const std::string& name, RC::Unreal::UDataTable* datatable)
    {
        m_datatableMap.insert_or_assign(name, datatable);

        std::lock_guard<std::mutex> guard(m_mutex);
        for (auto& [callbackId, callback] : m_callbackMap)
        {
            callback(datatable);
        }
    }

    void UDataTableRegistry::Add(RC::Unreal::UDataTable* datatable)
    {
        auto name = datatable->GetNamePrivate().ToString();
        Add(RC::to_string(name), datatable);
    }

    void UDataTableRegistry::Initialize()
    {
        TArray<UObject*> results;

        auto datatableClass = RC::Unreal::UDataTable::StaticClass();
        if (!datatableClass)
        {
            PS::Log<LogLevel::Error>(STR("Unable to initialize UDataTableRegistry, failed to get /Script/Engine.DataTable\n"));
            return;
        }

        PS::Log<LogLevel::Verbose>(STR("UClass for UDataTable found, fetching UDataTables...\n"));
        UECustom::UObjectGlobals::GetObjectsOfClass(datatableClass, results);

        int addedDatatables = 0;
        for (auto& object : results)
        {
            auto datatable = static_cast<RC::Unreal::UDataTable*>(object);
            auto name = object->GetNamePrivate().ToString();
            Add(RC::to_string(name), datatable);
            addedDatatables++;
        }

        PS::Log<LogLevel::Verbose>(STR("Finished mapping {} UDataTables.\n"), addedDatatables);
    }

    DatatableSerializeCallbackId UDataTableRegistry::GenerateDatatableSerializeCallbackId()
    {
        static RC::Unreal::uint64 datatableSerializeCallbackId = 0;
        return datatableSerializeCallbackId++;
    }
}