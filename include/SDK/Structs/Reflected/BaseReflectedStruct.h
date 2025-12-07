#pragma once

#include <memory>
#include "Unreal/FProperty.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Helpers/String.hpp"
#include "SDK/Helper/PropertyHelper.h"
#include "SDK/Structs/Custom/FScriptArrayHelper.h"

namespace UECustom {
    struct BaseReflectedStruct {
    public:
        BaseReflectedStruct(RC::Unreal::UScriptStruct* scriptStruct);
        BaseReflectedStruct(RC::Unreal::UScriptStruct* scriptStruct, void* data);
        virtual ~BaseReflectedStruct();

        void DestroyStruct();

        void* GetData();

        virtual RC::Unreal::UScriptStruct* StaticStruct() = 0;
    protected:
        template <typename T>
        void SetPropertyValue(RC::Unreal::FProperty* property, const T& value)
        {
            auto valuePtr = property->ContainerPtrToValuePtr<T>(m_data);
            *valuePtr = value;
        }

        template <typename T>
        T GetPropertyValue(RC::Unreal::FProperty* property)
        {
            auto valuePtr = property->ContainerPtrToValuePtr<T>(m_data);
            return *valuePtr;
        }

        // This will return a nullptr if the property doesn't exist.
        RC::Unreal::FProperty* GetProperty(const RC::StringType& propertyName)
        {
            auto property = Palworld::PropertyHelper::GetPropertyByName(m_scriptStruct, propertyName);
            if (!property)
            {
                return nullptr;
            }

            return property;
        }

        // This will throw an error if the property doesn't exist or if the type didn't match what was supplied.
        template <RC::Unreal::FFieldDerivative T>
        T* GetPropertyChecked(const RC::StringType& propertyName)
        {
            auto property = Palworld::PropertyHelper::GetPropertyByName(m_scriptStruct, propertyName);
            if (!property)
            {
                throw std::runtime_error(RC::fmt("Property '%S' does not exist in struct '%S'.", propertyName.c_str(), 
                    m_scriptStruct->GetNamePrivate().ToString().c_str()));
            }

            T* returnValue = RC::Unreal::CastField<T>(property);
            if (!returnValue)
            {
                throw std::runtime_error(RC::fmt("Property '%S' has the wrong type, expected '%S'.", propertyName.c_str(), *property->GetCPPType()));
            }

            return returnValue;
        }

        std::unique_ptr<FScriptArrayHelper> GetArrayPropertyValue(RC::Unreal::FProperty* property);
    private:
        RC::Unreal::UScriptStruct* m_scriptStruct = nullptr;
        void* m_data = nullptr;
    };
}