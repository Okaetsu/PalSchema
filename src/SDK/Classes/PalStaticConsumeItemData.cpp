#include "SDK/Classes/PalStaticConsumeItemData.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/CoreUObject/UObject/Class.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	UClass* UPalStaticConsumeItemData::StaticClass()
	{
		auto Class = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Pal.PalStaticConsumeItemData"));
		return Class;
	}
}