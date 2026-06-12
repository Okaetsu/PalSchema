#include "SDK/Classes/PalEnum.h"
#include "Helpers/Casting.hpp"

using namespace RC;

namespace Palworld {
    EPropertyType UPalEnum::GetUnderlyingType()
    {
        return *Helper::Casting::ptr_cast<EPropertyType*>(this, 0x40);
    }
}