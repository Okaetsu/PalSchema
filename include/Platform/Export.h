#pragma once

#if defined(_WIN32)
    #if defined(PALSCHEMA_BUILDING_DLL)
        #define PALSCHEMA_API __declspec(dllexport)
    #else
        #define PALSCHEMA_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define PALSCHEMA_API __attribute__((visibility("default")))
#else
    #define PALSCHEMA_API
#endif
