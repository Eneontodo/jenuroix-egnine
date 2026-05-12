cmake_minimum_required(VERSION 3.20)
project(JenuroixEngine VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Paths ──────────────────────────────────────────────────────────────────────
set(SRC_DIR   ${CMAKE_SOURCE_DIR}/src)
set(VEND_DIR  ${CMAKE_SOURCE_DIR}/vendor)
set(TOOLS_DIR ${CMAKE_SOURCE_DIR}/tools)

# ── Platform flags ─────────────────────────────────────────────────────────────
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN
                            _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
    if(MINGW)
        add_compile_options(-Wall -Wextra -O2
                            -Wno-unused-parameter -Wno-missing-field-initializers)
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    elseif(MSVC)
        add_compile_options(/W3 /utf-8 /MP /EHsc /wd4819)
    endif()
endif()

# ═══════════════════════════════════════════════════════════════════════════════
#  STEP 1: Run embed_assets.py BEFORE collecting source files
#  This generates:
#    src/EmbeddedAssets.h
#    src/EmbeddedAssets.cpp
# ═══════════════════════════════════════════════════════════════════════════════
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(EMBED_PY     ${TOOLS_DIR}/embed_assets.py)
set(ASSETS_DIR   ${CMAKE_SOURCE_DIR}/assets)
set(EMBED_CPP    ${SRC_DIR}/EmbeddedAssets.cpp)
set(EMBED_H      ${SRC_DIR}/EmbeddedAssets.h)

# We use a custom target + OUTPUT to get correct incremental rebuilds.
# The .stamp file is touched whenever the script runs successfully.
set(EMBED_STAMP  ${CMAKE_BINARY_DIR}/embed_assets.stamp)

# Glob all asset files so CMake knows when to re-run the script.
file(GLOB_RECURSE ASSET_FILES "${ASSETS_DIR}/*")

add_custom_command(
    OUTPUT  ${EMBED_CPP} ${EMBED_H} ${EMBED_STAMP}
    COMMAND ${Python3_EXECUTABLE} ${EMBED_PY}
                ${ASSETS_DIR}
                ${EMBED_CPP}
                ${EMBED_H}
    COMMAND ${CMAKE_COMMAND} -E touch ${EMBED_STAMP}
    DEPENDS ${EMBED_PY} ${ASSET_FILES}
    COMMENT "Embedding assets into EmbeddedAssets.cpp / .h ..."
    VERBATIM
)

add_custom_target(embed_assets_target DEPENDS ${EMBED_CPP} ${EMBED_H})

# ═══════════════════════════════════════════════════════════════════════════════
#  Vendor libraries
# ═══════════════════════════════════════════════════════════════════════════════

# GLAD
add_library(glad STATIC ${VEND_DIR}/glad/src/glad.c)
target_include_directories(glad PUBLIC ${VEND_DIR}/glad/include)
if(WIN32)
    target_link_libraries(glad PUBLIC opengl32)
endif()

# GLFW
if(EXISTS "${VEND_DIR}/glfw/CMakeLists.txt")
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    add_subdirectory(${VEND_DIR}/glfw)
    set(GLFW_TARGET glfw)
else()
    find_package(glfw3 3.3 REQUIRED)
    set(GLFW_TARGET glfw)
endif()

# GLM (header-only)
add_library(glm INTERFACE)
foreach(GLM_TRY "${VEND_DIR}/glm/glm/glm.hpp" "${VEND_DIR}/glm/glm.hpp")
    if(EXISTS ${GLM_TRY})
        target_include_directories(glm INTERFACE
            ${VEND_DIR}/glm)
        break()
    endif()
endforeach()

# STB (header-only — implementation is in ModelLoader.cpp)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${VEND_DIR}/stb)

# ImGui
set(IMGUI_DIR ${VEND_DIR}/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC ${GLFW_TARGET} glad)
if(MINGW)
    target_compile_options(imgui PRIVATE -w)
elseif(MSVC)
    target_compile_options(imgui PRIVATE /W0)
endif()

# ── Jolt Physics (optional) ───────────────────────────────────────────────────
# Clone: git clone https://github.com/jrouwe/JoltPhysics vendor/JoltPhysics
set(JOLT_DIR ${VEND_DIR}/JoltPhysics)

# Jolt keeps its CMakeLists.txt in Build/ subfolder
set(JOLT_CMAKE_DIR "")
foreach(JOLT_TRY
    "${JOLT_DIR}/Build/CMakeLists.txt"
    "${JOLT_DIR}/CMakeLists.txt")
    if(EXISTS ${JOLT_TRY})
        get_filename_component(JOLT_CMAKE_DIR ${JOLT_TRY} DIRECTORY)
        break()
    endif()
endforeach()

if(EXISTS "${JOLT_DIR}/Jolt/Jolt.h" AND NOT "${JOLT_CMAKE_DIR}" STREQUAL "")
    message(STATUS "  Jolt Physics : found (${JOLT_CMAKE_DIR})")
    set(JOLT_AVAILABLE ON)

    set(JOLT_BUILD_SHARED_LIBS      OFF CACHE BOOL "" FORCE)
    set(JOLT_ENABLE_ASSERTS         OFF CACHE BOOL "" FORCE)
    set(JOLT_DEBUG_RENDERER         OFF CACHE BOOL "" FORCE)
    set(JOLT_PROFILER_ENABLED       OFF CACHE BOOL "" FORCE)
    set(JOLT_OBJECT_LAYER_BITS      16  CACHE STRING "" FORCE)
    # Prevent Jolt from setting INTERFACE_INCLUDE_DIRECTORIES with absolute paths
    # that CMake rejects when they're inside the source tree
    set(JOLT_INSTALL                OFF CACHE BOOL "" FORCE)

    add_subdirectory(${JOLT_CMAKE_DIR} ${CMAKE_BINARY_DIR}/JoltPhysics EXCLUDE_FROM_ALL)
    set(JOLT_TARGET Jolt)

    # Jolt sets INTERFACE_INCLUDE_DIRECTORIES to an absolute path inside the
    # source tree which newer CMake rejects. Override it with a generator expression.
    set_target_properties(Jolt PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
            "$<BUILD_INTERFACE:${JOLT_DIR}>")

    if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(Jolt PRIVATE -w)
    elseif(MSVC)
        target_compile_options(Jolt PRIVATE /W0)
    endif()
else()
    if(EXISTS "${JOLT_DIR}/Jolt/Jolt.h")
        message(WARNING "  Jolt Physics : headers found but no CMakeLists.txt!")
        message(STATUS  "    Try: git clone https://github.com/jrouwe/JoltPhysics vendor/JoltPhysics")
    else()
        message(STATUS "  Jolt Physics : NOT found (physics will be stub)")
        message(STATUS "    To enable: git clone https://github.com/jrouwe/JoltPhysics vendor/JoltPhysics")
    endif()
    set(JOLT_AVAILABLE OFF)
    set(JOLT_TARGET "")
endif()

# ── miniaudio (optional, header-only) ────────────────────────────────────────
# Download: https://miniaud.io  →  vendor/miniaudio/miniaudio.h
set(MINIAUDIO_H ${VEND_DIR}/miniaudio/miniaudio.h)
if(EXISTS ${MINIAUDIO_H})
    message(STATUS "  miniaudio    : found")
    add_library(miniaudio INTERFACE)
    target_include_directories(miniaudio INTERFACE ${VEND_DIR}/miniaudio)
    set(MINIAUDIO_TARGET miniaudio)
else()
    message(STATUS "  miniaudio    : NOT found (audio will be stub)")
    message(STATUS "    To enable: download miniaudio.h from https://miniaud.io → vendor/miniaudio/miniaudio.h")
    set(MINIAUDIO_TARGET "")
endif()

# ═══════════════════════════════════════════════════════════════════════════════
#  Engine source files
# ═══════════════════════════════════════════════════════════════════════════════
file(GLOB_RECURSE ENGINE_SOURCES
    ${SRC_DIR}/core/*.cpp
    ${SRC_DIR}/render/*.cpp
    ${SRC_DIR}/game/*.cpp
    ${SRC_DIR}/ecs/*.cpp
    ${SRC_DIR}/ui/*.cpp
    ${SRC_DIR}/res/*.cpp
    ${SRC_DIR}/audio/*.cpp
    ${SRC_DIR}/physics/*.cpp
    ${SRC_DIR}/editor/*.cpp
)

# Exclude example files that define their own main()
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/editor/[0-9]+_.*\\.cpp$")

set(ENGINE_INCLUDES
    ${SRC_DIR}
    ${VEND_DIR}/stb
    ${VEND_DIR}/nlohmann
    ${VEND_DIR}/tinygltf
    ${VEND_DIR}/tinyobjloader
)
if(JOLT_AVAILABLE)
    list(APPEND ENGINE_INCLUDES ${JOLT_DIR})
endif()
if(MINIAUDIO_TARGET)
    list(APPEND ENGINE_INCLUDES ${VEND_DIR}/miniaudio)
endif()

set(ENGINE_DEFINES
    TINYGLTF_NO_STB_IMAGE_WRITE
    TINYGLTF_NO_INCLUDE_JSON
    GLM_ENABLE_EXPERIMENTAL
)

# ═══════════════════════════════════════════════════════════════════════════════
#  Main executable
# ═══════════════════════════════════════════════════════════════════════════════
add_executable(game
    ${SRC_DIR}/main.cpp
    ${ENGINE_SOURCES}
    ${EMBED_CPP}          # ← generated embedded asset source
)

# Must generate embedded assets before compiling game
add_dependencies(game embed_assets_target)

target_include_directories(game PRIVATE ${ENGINE_INCLUDES})
target_compile_definitions(game PRIVATE ${ENGINE_DEFINES})
target_link_libraries(game PRIVATE glad ${GLFW_TARGET} glm stb imgui)
if(JOLT_AVAILABLE)
    target_link_libraries(game PRIVATE ${JOLT_TARGET})
endif()
if(MINIAUDIO_TARGET)
    target_link_libraries(game PRIVATE ${MINIAUDIO_TARGET})
endif()

if(WIN32)
    target_link_libraries(game PRIVATE opengl32 gdi32 user32 shell32)
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(game PRIVATE GL dl pthread)
endif()
if(APPLE)
    target_link_libraries(game PRIVATE
        "-framework OpenGL" "-framework Cocoa"
        "-framework IOKit"  "-framework CoreVideo")
endif()

# ═══════════════════════════════════════════════════════════════════════════════
#  Editor executable
# ═══════════════════════════════════════════════════════════════════════════════
add_executable(editor
    ${SRC_DIR}/editor_app/editor_main.cpp
    ${ENGINE_SOURCES}
    ${EMBED_CPP}
)

add_dependencies(editor embed_assets_target)

target_include_directories(editor PRIVATE ${ENGINE_INCLUDES})
target_compile_definitions(editor PRIVATE ${ENGINE_DEFINES})
target_link_libraries(editor PRIVATE glad ${GLFW_TARGET} glm stb imgui)
if(JOLT_AVAILABLE)
    target_link_libraries(editor PRIVATE ${JOLT_TARGET})
endif()
if(MINIAUDIO_TARGET)
    target_link_libraries(editor PRIVATE ${MINIAUDIO_TARGET})
endif()
if(WIN32)
    target_link_libraries(editor PRIVATE opengl32 gdi32 user32 shell32)
    # WIN32 subsystem hides console — keep console for build log output
    set_target_properties(editor PROPERTIES LINK_FLAGS "")
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(editor PRIVATE GL dl pthread)
endif()
if(APPLE)
    target_link_libraries(editor PRIVATE
        "-framework OpenGL" "-framework Cocoa"
        "-framework IOKit"  "-framework CoreVideo")
endif()

message(STATUS "  Editor     : enabled → build target 'editor'")

# ═══════════════════════════════════════════════════════════════════════════════
#  Editor example programs (files matching src/editor/NN_*.cpp)
#  Each has its own main() and is built as a standalone executable.
# ═══════════════════════════════════════════════════════════════════════════════
file(GLOB EDITOR_EXAMPLE_FILES ${SRC_DIR}/editor/[0-9]*.cpp)
foreach(SRC ${EDITOR_EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${SRC} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${SRC} ${ENGINE_SOURCES} ${EMBED_CPP})
    add_dependencies(${EXAMPLE_NAME} embed_assets_target)
    target_include_directories(${EXAMPLE_NAME} PRIVATE ${ENGINE_INCLUDES})
    target_compile_definitions(${EXAMPLE_NAME} PRIVATE ${ENGINE_DEFINES})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE glad ${GLFW_TARGET} glm stb imgui)
    if(JOLT_AVAILABLE)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE ${JOLT_TARGET})
    endif()
    if(MINIAUDIO_TARGET)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE ${MINIAUDIO_TARGET})
    endif()
    if(WIN32)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE opengl32 gdi32 user32 shell32)
    endif()
    if(UNIX AND NOT APPLE)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE GL dl pthread)
    endif()
    if(APPLE)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE
            "-framework OpenGL" "-framework Cocoa"
            "-framework IOKit"  "-framework CoreVideo")
    endif()
    message(STATUS "  Example    : ${EXAMPLE_NAME}")
endforeach()

# ═══════════════════════════════════════════════════════════════════════════════
# ═══════════════════════════════════════════════════════════════════════════════
option(BUILD_EXAMPLES "Build example programs" OFF)
if(BUILD_EXAMPLES AND EXISTS "${CMAKE_SOURCE_DIR}/examples")
    file(GLOB EXAMPLE_FILES ${CMAKE_SOURCE_DIR}/examples/*.cpp)
    foreach(SRC ${EXAMPLE_FILES})
        get_filename_component(NAME ${SRC} NAME_WE)
        add_executable(${NAME} ${SRC} ${ENGINE_SOURCES} ${EMBED_CPP})
        add_dependencies(${NAME} embed_assets_target)
        target_include_directories(${NAME} PRIVATE ${ENGINE_INCLUDES})
        target_compile_definitions(${NAME} PRIVATE ${ENGINE_DEFINES})
        target_link_libraries(${NAME} PRIVATE glad ${GLFW_TARGET} glm stb imgui)
        if(WIN32)
            target_link_libraries(${NAME} PRIVATE opengl32 gdi32 user32 shell32)
        endif()
        if(UNIX AND NOT APPLE)
            target_link_libraries(${NAME} PRIVATE GL dl pthread)
        endif()
    endforeach()
endif()

# ═══════════════════════════════════════════════════════════════════════════════
#  Summary
# ═══════════════════════════════════════════════════════════════════════════════
message(STATUS "")
message(STATUS "  Platform  : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler  : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build     : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Python    : ${Python3_EXECUTABLE}")
message(STATUS "  Assets    : ${ASSETS_DIR}")
message(STATUS "  Embedded  : ${EMBED_CPP}")
message(STATUS "  Examples  : ${BUILD_EXAMPLES}")
message(STATUS "")
