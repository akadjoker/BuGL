cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0072 NEW)

project(bugl VERSION 2.0.0)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUGL_PORTABLE_RELEASE "Build release binaries without CPU-specific native flags" ON)
option(BUGL_STRIP_RELEASE "Strip symbols from release binaries when supported" ON)
option(BUGL_IMGUI_DEMO "Build Dear ImGui demo window support" OFF)
option(BUGL_ENABLE_COMPILER_CACHE "Use compiler cache launcher (sccache/ccache) when available" ON)

if(BUGL_ENABLE_COMPILER_CACHE)
    if(NOT CMAKE_C_COMPILER_LAUNCHER AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
        find_program(BUGL_COMPILER_CACHE_PROGRAM NAMES sccache ccache)
        if(BUGL_COMPILER_CACHE_PROGRAM)
            message(STATUS "BuGL: enabling compiler cache launcher: ${BUGL_COMPILER_CACHE_PROGRAM}")
            set(CMAKE_C_COMPILER_LAUNCHER "${BUGL_COMPILER_CACHE_PROGRAM}" CACHE STRING "C compiler launcher" FORCE)
            set(CMAKE_CXX_COMPILER_LAUNCHER "${BUGL_COMPILER_CACHE_PROGRAM}" CACHE STRING "CXX compiler launcher" FORCE)
        else()
            message(STATUS "BuGL: compiler cache launcher not found (sccache/ccache)")
        endif()
    endif()
endif()

# Embutir as bibliotecas C/C++ padrão no executável Windows para evitar missing DLLs

find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
    add_custom_target(
        tidy
        COMMAND ${CLANG_TIDY_EXE}
        -p=${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libbu/src/*.cpp
        ${CMAKE_SOURCE_DIR}/main/src/*.cpp
        COMMENT "Running clang-tidy"
    )
endif()

add_subdirectory(vendor/imgui)
# Forçar Position Independent Code para a library poder ser linkada num plugin partilhado
if(TARGET imgui)
    set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
add_subdirectory(vendor/miniz)
if(TARGET miniz)
    set_target_properties(miniz PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
add_subdirectory(vendor/poly2tri)
add_subdirectory(vendor/box2d)
if(TARGET box2d)
    target_compile_definitions(box2d PUBLIC B2_USER_SETTINGS)
    target_include_directories(box2d PUBLIC ${CMAKE_SOURCE_DIR}/BuBox2d/include)
    set_target_properties(box2d PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
add_subdirectory(vendor/audio)
set(OPENSTEER_BUILD_DEMO OFF CACHE BOOL "Disable upstream OpenSteer demo in BuGL build" FORCE)
add_subdirectory(vendor/OpenSteer)
add_subdirectory(vendor/Jolt)

# Embed Chipmunk2D as a dependency only.
set(BUILD_DEMOS OFF CACHE BOOL "Disable Chipmunk2D demos in BuGL build" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "Disable Chipmunk2D shared library in BuGL build" FORCE)
set(BUILD_STATIC ON CACHE BOOL "Enable Chipmunk2D static library in BuGL build" FORCE)
set(INSTALL_STATIC OFF CACHE BOOL "Disable Chipmunk2D install target in BuGL build" FORCE)
add_subdirectory(vendor/Chipmunk2D)
if(TARGET chipmunk_static)
    target_include_directories(chipmunk_static PUBLIC ${CMAKE_SOURCE_DIR}/vendor/Chipmunk2D/include)
    set_target_properties(chipmunk_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()


# Embed ODE as a dependency only (skip upstream demos/tests targets).
set(ODE_WITH_DEMOS OFF CACHE BOOL "Disable ODE demos in BuGL build" FORCE)
set(ODE_WITH_TESTS OFF CACHE BOOL "Disable ODE tests in BuGL build" FORCE)
add_subdirectory(vendor/ODE)

# Assimp - import only, no tools/tests/exporter
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/assimp)

#Configurar o SDL2 para compilar apenas de forma estática
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "Disable SDL2 install rules in BuGL build" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Disable SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Enable SDL static library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Disable SDL test library in BuGL build" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Disable SDL test programs in BuGL build" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "Disable SDL audio subsystem in BuGL build" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "Disable SDL haptic subsystem in BuGL build" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "Disable SDL sensor subsystem in BuGL build" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "Disable SDL Vulkan support in BuGL build" FORCE)
set(SDL_KMSDRM OFF CACHE BOOL "Disable SDL KMSDRM video backend in BuGL build" FORCE)
set(SDL_KMSDRM_SHARED OFF CACHE BOOL "Disable SDL KMSDRM shared loading in BuGL build" FORCE)
set(SDL_RPI OFF CACHE BOOL "Disable SDL Raspberry Pi backend in BuGL build" FORCE)
set(SDL_VIVANTE OFF CACHE BOOL "Disable SDL Vivante backend in BuGL build" FORCE)
set(SDL_OFFSCREEN OFF CACHE BOOL "Disable SDL offscreen backend in BuGL build" FORCE)
add_subdirectory(vendor/SDL)


add_subdirectory(libbu)
add_subdirectory(main)

# Language test suite (lightweight: only needs libbu)
enable_testing()
add_subdirectory(tests)

add_subdirectory(BuOde)
add_subdirectory(BuBox2d)
add_subdirectory(BuJolt)
add_subdirectory(BuChipmunk)
add_subdirectory(BuOpenSteer)
add_subdirectory(BuRecast)
add_subdirectory(BuMicroPather)
add_subdirectory(BuAssimp)
add_subdirectory(BuMeshOpt)
 

 
