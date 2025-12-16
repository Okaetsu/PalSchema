#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "SDK/Classes/PalStaticItemDataBase.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	RC::Unreal::UClass* UPalStaticItemDataBase::StaticClass()
	{
		static auto Class = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, TEXT("/Script/Pal.PalStaticItemDataBase"));
		return Class;
	}
}
