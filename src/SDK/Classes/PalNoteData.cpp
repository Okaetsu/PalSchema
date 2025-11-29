#include "SDK/Classes/PalNoteData.h"
#include "Unreal/UClass.hpp"
#include "Unreal/UObjectGlobals.hpp"

using namespace RC::Unreal;

namespace Palworld {
    UClass* UPalNoteData::StaticClass()
    {
        static auto Class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, TEXT("/Script/Pal.PalNoteData"));
        return Class;
    }
}