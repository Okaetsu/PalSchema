#pragma once

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace PS::Platform
{
    inline bool IsRunningUnderWine()
    {
#if defined(_WIN32)
        auto ntdll = GetModuleHandleW(L"ntdll.dll");
        return ntdll && GetProcAddress(ntdll, "wine_get_version");
#else
        return false;
#endif
    }
}
