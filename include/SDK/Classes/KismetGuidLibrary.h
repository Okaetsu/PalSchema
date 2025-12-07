#pragma once

#include "Unreal/UObject.hpp"
#include "SDK/Structs/Guid.h"

namespace UECustom {
    class UKismetGuidLibrary : public RC::Unreal::UObject {
    public:
        static UECustom::FGuid NewGuid();

        static RC::Unreal::FString Conv_GuidToString(const UECustom::FGuid& Guid);
    private:
        static UKismetGuidLibrary* GetDefaultObj();
    };
}