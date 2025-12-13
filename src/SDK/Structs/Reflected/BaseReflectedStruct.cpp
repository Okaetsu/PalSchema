#include "SDK/Structs/Reflected/BaseReflectedStruct.h"
#include "Unreal/Core/Containers/ScriptArray.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    BaseReflectedStruct::BaseReflectedStruct(RC::Unreal::UScriptStruct* scriptStruct) : m_scriptStruct(scriptStruct)
    {
        m_data = FMemory::Malloc(m_scriptStruct->GetStructureSize());
        m_scriptStruct->InitializeStruct(m_data);
    }

    BaseReflectedStruct::BaseReflectedStruct(RC::Unreal::UScriptStruct* scriptStruct, void* data) : m_scriptStruct(scriptStruct), m_data(data)
    {
    }

    BaseReflectedStruct::~BaseReflectedStruct()
    {
    }

    void BaseReflectedStruct::DestroyStruct()
    {
        m_scriptStruct->DestroyStruct(m_data);
        FMemory::Free(m_data);
    }

    void* BaseReflectedStruct::GetData()
    {
        return m_data;
    }

    std::unique_ptr<FScriptArrayHelper> BaseReflectedStruct::GetArrayPropertyValue(RC::Unreal::FProperty* property)
    {
        auto arrayProperty = CastField<FArrayProperty>(property);
        if (!arrayProperty)
        {
            throw std::runtime_error(RC::fmt("Tried accessing property '%S' as an array property, but property was another type.", property->GetName().c_str()));
        }

        auto arrayData = arrayProperty->ContainerPtrToValuePtr<void>(m_data);
        auto scriptArray = static_cast<FScriptArray*>(arrayData);
        auto scriptArrayHelper = std::make_unique<UECustom::FScriptArrayHelper>(scriptArray, arrayProperty);

        return std::move(scriptArrayHelper);
    }
}