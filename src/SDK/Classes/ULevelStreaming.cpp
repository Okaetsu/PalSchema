#include "SDK/Classes/ULevelStreaming.h"
#include "Helpers/Casting.hpp"
#include "Unreal/FWeakObjectPtr.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    UWorldPartition* ULevelStreaming::GetOuterWorldPartition()
    {
        auto WeakPtr = *Helper::Casting::ptr_cast<TWeakObjectPtr<UWorldPartition, FWeakObjectPtr>*>(this, 0x1BC);
        auto InnerObject = WeakPtr.Get();
        if (!InnerObject)
        {
            return nullptr;
        }

        return InnerObject;
    }
}