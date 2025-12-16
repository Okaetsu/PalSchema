#include "SDK/Classes/PalNoteData.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/CoreUObject/UObject/Class.hpp"

using namespace RC::Unreal;

namespace Palworld {
    UClass* UPalNoteData::StaticClass()
    {
        static auto Class = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, TEXT("/Script/Pal.PalNoteData"));
        return Class;
    }
}