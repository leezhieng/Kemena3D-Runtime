cmake_minimum_required(VERSION 3.16)
project(Kemena3DRuntime)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

# Path to the Kemena3D SDK output directory (replaced by build_runtime.py).
set(KEMENA3D_SDK_DIR "C:/Kemena3D-SDK/" CACHE PATH "Path to Kemena3D SDK output directory")
get_filename_component(KEMENA3D_SOURCE_DIR "${KEMENA3D_SDK_DIR}" DIRECTORY)

# Fail fast with a clear message if the SDK path is wrong, instead of a cryptic
# "cannot open kemena/kemena.h" later. The SDK installs headers under
# <dir>/Debug/include and <dir>/Release/include.
if(NOT EXISTS "${KEMENA3D_SDK_DIR}/Debug/include/kemena/kemena.h"
   AND NOT EXISTS "${KEMENA3D_SDK_DIR}/Release/include/kemena/kemena.h")
    message(FATAL_ERROR
        "KEMENA3D_SDK_DIR='${KEMENA3D_SDK_DIR}' is not a built Kemena3D SDK.\n"
        "Expected '<dir>/Debug/include/kemena/kemena.h' or '<dir>/Release/include/kemena/kemena.h'.\n"
        "Build the SDK first (Kemena3D-SDK/build_sdk.py) and point this at its Output folder, e.g.\n"
        "  -DKEMENA3D_SDK_DIR=\"D:/Projects/Kemena3D/Kemena3D-SDK/Output\"")
endif()

option(KEMENA3D_LINK_STATIC "Link against the static Kemena3D SDK library" ON)

# The runtime is a single generic player; no editor sources.
set(SOURCES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

# Release: GUI subsystem (no console window). Debug keeps the console for logs.
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE "$<$<CONFIG:Release>:TRUE>"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release"
    RUNTIME_OUTPUT_DIRECTORY         "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:${KEMENA3D_SDK_DIR}/Debug/include>"
    "$<$<CONFIG:Release>:${KEMENA3D_SDK_DIR}/Release/include>"
    "$<$<CONFIG:Debug>:${KEMENA3D_SOURCE_DIR}/Dependencies/assimp/build_Debug/include>"
    "$<$<CONFIG:Release>:${KEMENA3D_SOURCE_DIR}/Dependencies/assimp/build_Release/include>"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<BOOL:${KEMENA3D_LINK_STATIC}>:KEMENA3D_STATIC>
    NOMINMAX
)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    # Release uses the GUI subsystem (WIN32_EXECUTABLE) to suppress the console
    # window, but the entry point is the standard int main(); tell the linker to
    # use the CRT's main startup instead of looking for WinMain.
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>
    )
endif()

target_link_directories(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:${KEMENA3D_SDK_DIR}/Debug/lib>"
    "$<$<CONFIG:Release>:${KEMENA3D_SDK_DIR}/Release/lib>"
)

# --- Platform link lists ----------------------------------------------------
# NOTE: this mirrors the editor's link setup, which is currently Windows-first
# (MSVC + MinGW). Linux/macOS targets need their own dependency + system-lib
# branches here once those dependency builds are wired in download_dep.py.
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Kemena3DSDK
        opengl32 winmm ole32 setupapi gdi32 imm32 version oleaut32 uuid
    )

    if(MSVC)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            SDL3-static
            $<$<CONFIG:Debug>:assimp-vc143-mtd>
            $<$<CONFIG:Release>:assimp-vc143-mt>
            $<$<CONFIG:Debug>:angelscriptd>
            $<$<CONFIG:Release>:angelscript>
            imgui
            imguizmo
            $<$<CONFIG:Debug>:libglew32d>
            $<$<CONFIG:Release>:libglew32>
            $<$<CONFIG:Debug>:zlibstaticd>
            $<$<CONFIG:Release>:zlibstatic>
            $<$<CONFIG:Debug>:Recast-d>
            $<$<CONFIG:Debug>:Detour-d>
            $<$<CONFIG:Debug>:DetourCrowd-d>
            $<$<CONFIG:Debug>:DetourTileCache-d>
            $<$<CONFIG:Release>:Recast>
            $<$<CONFIG:Release>:Detour>
            $<$<CONFIG:Release>:DetourCrowd>
            $<$<CONFIG:Release>:DetourTileCache>
            "$<$<CONFIG:Debug>:${KEMENA3D_SOURCE_DIR}/Dependencies/jolt/Build/build_Debug/Debug/Jolt.lib>"
            "$<$<CONFIG:Release>:${KEMENA3D_SOURCE_DIR}/Dependencies/jolt/Build/build_Release/Release/Jolt.lib>"
        )
    else()
        # MinGW
        target_link_libraries(${PROJECT_NAME} PRIVATE
            SDL3
            $<$<CONFIG:Debug>:assimpd>
            $<$<CONFIG:Release>:assimp>
            angelscript
            imgui
            imguizmo
            $<$<CONFIG:Debug>:libglew32d>
            $<$<CONFIG:Release>:libglew32>
            zlibstatic
            "$<$<CONFIG:Debug>:${KEMENA3D_SOURCE_DIR}/Dependencies/recast/dist/lib/Debug/libRecast.a>"
            "$<$<CONFIG:Debug>:${KEMENA3D_SOURCE_DIR}/Dependencies/recast/dist/lib/Debug/libDetour.a>"
            "$<$<CONFIG:Debug>:${KEMENA3D_SOURCE_DIR}/Dependencies/recast/dist/lib/Debug/libDetourCrowd.a>"
            "$<$<CONFIG:Debug>:${KEMENA3D_SOURCE_DIR}/Dependencies/recast/dist/lib/Debug/libDetourTileCache.a>"
            "$<$<CONFIG:Release>:${KEMENA3D_SOURCE_DIR}/Dependencies/recast/dist/lib/Release/libRecast.a>"
            "$<$<CONFIG:Release>:${KEMENA3D_SOURCE_DIR}/Dependencies/recast/dist/lib/Release/libDetour.a>"
            "$<$<CONFIG:Release>:${KEMENA3D_SOURCE_DIR}/Dependencies/recast/dist/lib/Release/libDetourCrowd.a>"
            "$<$<CONFIG:Release>:${KEMENA3D_SOURCE_DIR}/Dependencies/recast/dist/lib/Release/libDetourTileCache.a>"
            "$<$<CONFIG:Debug>:${KEMENA3D_SOURCE_DIR}/Dependencies/jolt/Build/build_Debug/libJolt.a>"
            "$<$<CONFIG:Release>:${KEMENA3D_SOURCE_DIR}/Dependencies/jolt/Build/build_Release/libJolt.a>"
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            -static-libstdc++ -static-libgcc
            $<$<CONFIG:Release>:-s>
        )
    endif()
elseif(APPLE)
    # TODO: macOS link list (clang). Frameworks: Cocoa, OpenGL, IOKit, CoreVideo.
    message(WARNING "macOS runtime link list not yet configured.")
    target_link_libraries(${PROJECT_NAME} PRIVATE Kemena3DSDK)
elseif(UNIX)
    # TODO: Linux link list (gcc). System: GL, X11, dl, pthread; deps as .a/.so.
    message(WARNING "Linux runtime link list not yet configured.")
    target_link_libraries(${PROJECT_NAME} PRIVATE Kemena3DSDK)
endif()
