#pragma once

#include "Unreal/UObject.hpp"

namespace UECustom {
    // Wrapper class for UObject which is overriding certain functions to use functions from PropertyHelper due to requiring the use of them early.
    // This avoids the need for custom UE4SS build since UE4SS' UE API doesn't initialize early enough yet.
    // Calling GetValuePtrByPropertyNameInChain from UE4SS' own API for example would just return an empty object or null when PalSchema needs to call it.
    class UObjectWrapper : public RC::Unreal::UObject {
    public:
        void* GetValuePtrByPropertyNameInChain(const RC::Unreal::TCHAR* PropertyName);

        template<RC::Unreal::UObjectPointerDerivativeOrAnyNonUObject ReturnType>
        ReturnType* GetValuePtrByPropertyNameInChain(const TCHAR* PropertyName)
        {
            return static_cast<ReturnType*>(UObjectWrapper::GetValuePtrByPropertyNameInChain(PropertyName));
        }
    };
}