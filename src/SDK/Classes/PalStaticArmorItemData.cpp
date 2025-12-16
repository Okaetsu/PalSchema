#include "SDK/Classes/PalStaticArmorItemData.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/CoreUObject/UObject/Class.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	UClass* UPalStaticArmorItemData::StaticClass()
	{
		auto Class = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Pal.PalStaticArmorItemData"));
		return Class;
	}
}