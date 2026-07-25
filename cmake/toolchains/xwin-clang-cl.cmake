# PalSchema wrapper around the pinned RE-UE4SS xwin toolchain.
#
# Keep the compiler and SDK behavior owned by RE-UE4SS. This wrapper only
# provides a distro-neutral default cache location when XWIN_DIR is unset.

if(NOT DEFINED ENV{XWIN_DIR} OR "$ENV{XWIN_DIR}" STREQUAL "")
    if(DEFINED ENV{XDG_CACHE_HOME} AND NOT "$ENV{XDG_CACHE_HOME}" STREQUAL "")
        set(_palschema_xwin_dir "$ENV{XDG_CACHE_HOME}/palschema/xwin")
    elseif(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
        set(_palschema_xwin_dir "$ENV{HOME}/.cache/palschema/xwin")
    else()
        message(FATAL_ERROR "Set XWIN_DIR because neither XDG_CACHE_HOME nor HOME is available.")
    endif()

    set(ENV{XWIN_DIR} "${_palschema_xwin_dir}")
endif()

include(
    "${CMAKE_CURRENT_LIST_DIR}/../../deps/RE-UE4SS/cmake/toolchains/xwin-clang-cl-toolchain.cmake"
)

# PalSchema and its pinned RE-UE4SS revision do not use C++ modules. Disabling
# dependency scanning also keeps CMake's nested IPO probe from losing the
# clang-scan-deps path when it creates its temporary cross-build project.
set(
    CMAKE_CXX_SCAN_FOR_MODULES
    OFF
    CACHE BOOL
    "Disable C++ module dependency scanning for the PalSchema xwin build"
    FORCE
)
