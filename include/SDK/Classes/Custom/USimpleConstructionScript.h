#pragma once

#include "SDK/Classes/Custom/UObjectWrapper.h"
#include "Core/Containers/Array.hpp"
#include "SDK/Classes/Custom/USCS_Node.h"

namespace UECustom {
    class USimpleConstructionScript : public UECustom::UObjectWrapper {
    public:
        RC::Unreal::TArray<USCS_Node*>& GetAllNodes();
    };
}