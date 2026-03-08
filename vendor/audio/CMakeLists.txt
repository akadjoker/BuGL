cmake_minimum_required(VERSION 3.20)

project(bugl_audio LANGUAGES C CXX)

add_library(bugl_audio STATIC
    src/miniaudio.c
    src/bugl_audio.cpp
)

target_include_directories(bugl_audio
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(bugl_audio PUBLIC cxx_std_17)

if (WIN32)
    target_link_libraries(bugl_audio PUBLIC winmm)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(bugl_audio PUBLIC pthread m dl)
endif()

