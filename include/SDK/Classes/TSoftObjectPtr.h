#pragma once

#include "SDK/Classes/TPersistentObjectPtr.h"
#include "SDK/Structs/FSoftObjectPtr.h"

namespace UECustom {
    template<typename UEType>
    class TSoftObjectPtr
    {
    public:
        FORCEINLINE TSoftObjectPtr() {};

        /** Construct from a soft object path */
        explicit FORCEINLINE TSoftObjectPtr(FSoftObjectPath ObjectPath) : SoftObjectPtr(ObjectPath)
        {
        }

        template <
            class U
            UE_REQUIRES(std::is_convertible_v<U*, UEType*>)
        >
        FORCEINLINE TSoftObjectPtr(const TSoftObjectPtr<U>& Other)
            : SoftObjectPtr(Other.SoftObjectPtr)
        {
        }

        const FSoftObjectPtr Get() const
        {
            return SoftObjectPtr;
        }
    private:
        FSoftObjectPtr SoftObjectPtr;
    };
}