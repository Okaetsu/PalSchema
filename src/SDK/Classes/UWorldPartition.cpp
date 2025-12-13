#include "SDK/Classes/UWorldPartition.h"
#include "Helpers/Casting.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    UWorld* UWorldPartition::GetWorld()
    {
        return *Helper::Casting::ptr_cast<UWorld**>(this, 0x70);
    }
}