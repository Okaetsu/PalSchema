#include "SDK/Helper/Memory.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Utility/Logging.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    uintptr_t** GetVTablePtrByClassPath(const RC::StringType& classPath)
    {
        auto classObject = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, classPath.c_str(), false);
        if (!classObject)
        {
            PS::Log<LogLevel::Error>(STR("Unable to get VTable pointer for {}, class not found.\n"), classPath);
            return nullptr;
        }

        auto& cdo = classObject->GetClassDefaultObject();
        if (!cdo)
        {
            PS::Log<LogLevel::Error>(STR("Unable to get VTable pointer for {}, class default object not found.\n"), classPath);
            return nullptr;
        }

        uintptr_t** vtablePtr = *(uintptr_t***)cdo;
        return vtablePtr;
    }

    void* GetVirtualFunctionFromVTable(uintptr_t** vtable, size_t offset)
    {
        void* vfuncptr = (void*)vtable[offset];
        return vfuncptr;
    }
}