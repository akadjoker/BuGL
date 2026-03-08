cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0072 NEW)

project(wdiv)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
    add_custom_target(
        tidy
        COMMAND ${CLANG_TIDY_EXE}
        -p=${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libwdiv/src/*.cpp
        ${CMAKE_SOURCE_DIR}/main/src/*.cpp
        COMMENT "Running clang-tidy"
    )
endif()

add_subdirectory(vendor/miniz)
add_subdirectory(vendor/poly2tri)
add_subdirectory(vendor/box2d)
add_subdirectory(vendor/audio)

# Embed ODE as a dependency only (skip upstream demos/tests targets).
set(ODE_WITH_DEMOS OFF CACHE BOOL "Disable ODE demos in BuGL build" FORCE)
set(ODE_WITH_TESTS OFF CACHE BOOL "Disable ODE tests in BuGL build" FORCE)
add_subdirectory(vendor/ODE)


add_subdirectory(libbu)
add_subdirectory(main)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/testAudio/CMakeLists.txt")
    add_subdirectory(testAudio)
endif()
 
 


 
