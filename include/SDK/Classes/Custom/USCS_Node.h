#pragma once

#include "SDK/Classes/Custom/UObjectWrapper.h"

namespace UECustom {
    class USCS_Node : public UECustom::UObjectWrapper {
    public:
        RC::Unreal::UObject* GetComponentTemplate();
    };
}