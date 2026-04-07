#pragma once

#include "Helpers/String.hpp"

namespace RC::Unreal {
    class UClass;
}

namespace Palworld {
    uintptr_t** GetVTablePtrByClassPath(const RC::StringType& classPath);
    void* GetVirtualFunctionFromClass(RC::Unreal::UClass* targetClass, size_t offset);
    void* GetVirtualFunctionFromVTable(uintptr_t** vtable, size_t offset);
}