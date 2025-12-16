#include "SDK/Classes/PalDynamicWeaponItemDataBase.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/CoreUObject/UObject/Class.hpp"

using namespace RC::Unreal;

namespace Palworld {
	UClass* UPalDynamicWeaponItemDataBase::StaticClass()
	{
		static auto Class = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Pal.PalDynamicWeaponItemDataBase"));
		return Class;
	}
}