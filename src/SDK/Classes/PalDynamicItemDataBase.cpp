#include "SDK/Classes/PalDynamicItemDataBase.h"

using namespace RC::Unreal;

namespace Palworld {
    bool& UPalDynamicItemDataBase::GetIgnoreOnSave()
    {
        auto bIgnoreOnSave = this->GetValuePtrByPropertyNameInChain<bool>(TEXT("bIgnoreOnSave"));
        return *bIgnoreOnSave;
    }
}