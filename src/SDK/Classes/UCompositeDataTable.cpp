#include "SDK/Classes/UCompositeDataTable.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Helpers/Casting.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    UClass* UCompositeDataTable::StaticClass()
    {
        static auto Class = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.CompositeDataTable"));
        return Class;
    }

    TArray<RC::Unreal::TObjectPtr<RC::Unreal::UDataTable>> UECustom::UCompositeDataTable::GetParentTables()
    {
        return *Helper::Casting::ptr_cast<TArray<TObjectPtr<RC::Unreal::UDataTable>>*>(this, 0xb0);
    }
}
