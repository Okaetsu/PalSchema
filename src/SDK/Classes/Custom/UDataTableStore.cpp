#include "SDK/Classes/UCompositeDataTable.h"
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
        auto parentDatatableIt = m_parentTableNameToCompositeDatatableMap.find(name);
        if (parentDatatableIt != m_parentTableNameToCompositeDatatableMap.end())
        {
            return parentDatatableIt->second;
        }

        auto datatableIt = m_datatableMap.find(name);
        if (datatableIt != m_datatableMap.end())
        {
            return datatableIt->second;
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

        if (datatable->GetClassPrivate() == UECustom::UCompositeDataTable::StaticClass())
        {
            auto compositeDatatable = static_cast<UECustom::UCompositeDataTable*>(datatable);
            for (auto& parentTable : compositeDatatable->GetParentTables())
            {
                m_parentTableNameToCompositeDatatableMap.emplace(RC::to_string(parentTable->GetName()), compositeDatatable);
            }
        }

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

    DatatableSerializeCallbackId UDataTableRegistry::GenerateDatatableSerializeCallbackId()
    {
        static RC::Unreal::uint64 datatableSerializeCallbackId = 0;
        return datatableSerializeCallbackId++;
    }
}