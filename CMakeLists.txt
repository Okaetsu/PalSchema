cmake_minimum_required(VERSION 3.22)

project(PalSchema LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps)

set(TARGET PalSchema)

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dllmain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Generator/JsonSchema/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Loader/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Loader/Blueprint/PalBlueprintMod.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Loader/Spawner/PalListInfo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Loader/Spawner/PalSpawnGroupListInfo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Loader/Spawner/SpawnerInfo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Loader/WildcardFilter/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Classes/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Classes/Custom/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Classes/Custom/DataTable/TableSerializer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Classes/Property/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Helper/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Structs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Structs/Reflected/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Structs/Custom/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/Structs/Custom/DataTable/FTableSerializerRow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Tools/EnumSchemaDefinitionGenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utility/Config.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utility/JsonHelpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utility/EnumHelpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Misc/FileWatchWrapper.cpp"
)

add_library(${TARGET} SHARED ${SRC_FILES})
if(WIN32)
    target_sources(${TARGET} PRIVATE version.rc)

    # The RE-UE4SS xwin toolchain configures SDK headers for clang-cl, but
    # CMake's llvm-rc preprocessing step needs the include paths separately.
    if(CMAKE_CROSSCOMPILING AND DEFINED XWIN_CACHE_DIR)
        set_property(
            SOURCE version.rc
            APPEND PROPERTY INCLUDE_DIRECTORIES
                "${XWIN_CACHE_DIR}/crt/include"
                "${XWIN_CACHE_DIR}/sdk/include/ucrt"
                "${XWIN_CACHE_DIR}/sdk/include/shared"
                "${XWIN_CACHE_DIR}/sdk/include/um"
                "${XWIN_CACHE_DIR}/sdk/include/winrt"
        )
    endif()
endif()

target_include_directories(${TARGET} PRIVATE
    include/
)
target_compile_definitions(${TARGET} PRIVATE PALSCHEMA_BUILDING_DLL=1)
target_link_libraries(${TARGET} PRIVATE Zydis Zycore safetyhook nlohmann_json::nlohmann_json glaze::glaze UE4SS efsw)

if(MSVC)
    # Keep Shipping DLLs reproducible across build times and checkout paths.
    # /Brepro replaces the wall-clock PE/PDB identity with a content-derived
    # value, while the alternate PDB path prevents the absolute build root
    # from leaking into the CodeView record embedded in the DLL.
    target_link_options(${TARGET} PRIVATE
        "/Brepro"
        "/PDBALTPATH:PalSchema.pdb"
    )
endif()
