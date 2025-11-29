#pragma once

#include "SDK/Classes/TPersistentObjectPtr.h"
#include "SDK/Structs/FSoftObjectPath.h"

namespace UECustom {
    class FSoftObjectPtr : public TPersistentObjectPtr<FSoftObjectPath>
    {
    public:
        explicit FORCEINLINE FSoftObjectPtr() : TPersistentObjectPtr<FSoftObjectPath>()
        {
        }

        explicit FORCEINLINE FSoftObjectPtr(const FSoftObjectPath& ObjectPath) : TPersistentObjectPtr<FSoftObjectPath>(ObjectPath)
        {
        }

        FORCEINLINE FSoftObjectPtr(const FSoftObjectPtr& ObjectPtr) : TPersistentObjectPtr<FSoftObjectPath>(ObjectPtr.ObjectID)
        {
        }
    };
}