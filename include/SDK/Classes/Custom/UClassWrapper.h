#pragma once

#include "Unreal/CoreUObject/UObject/Class.hpp"

namespace UECustom {
    class UClassWrapper : public RC::Unreal::UClass {
    public:
        RC::Unreal::UObject* GetDefaultObject(bool bCreateIfNeeded = true);

        // bForce Assemble the stream even if it has been already assembled (deletes the old one)
        void AssembleReferenceTokenStream(bool bForce = false);

        void StaticLink(bool bRelinkExistingProperties);
    };
}