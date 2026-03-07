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


add_subdirectory(libbu)
add_subdirectory(main)
 
 


 
