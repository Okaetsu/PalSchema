#include "SDK/Classes/PalDynamicArmorItemDataBase.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/CoreUObject/UObject/Class.hpp"

using namespace RC::Unreal;

namespace Palworld {
	UClass* UPalDynamicArmorItemDataBase::StaticClass()
	{
		static auto Class = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Pal.PalDynamicArmorItemDataBase"));
		return Class;
	}
}