cmake_minimum_required(VERSION 3.20)

project(testAudio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if (NOT TARGET bugl_audio)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../vendor/audio ${CMAKE_BINARY_DIR}/vendor_audio_build)
endif()

add_executable(testAudio src/main.cpp)
target_link_libraries(testAudio PRIVATE bugl_audio)

add_executable(demo_audio_procedural src/demo_audio_procedural.cpp)
target_link_libraries(demo_audio_procedural PRIVATE bugl_audio)
