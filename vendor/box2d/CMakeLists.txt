cmake_minimum_required(VERSION 3.10)
project(Box2D)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti -fno-strict-aliasing ")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



file(GLOB SOURCES "src/*.cpp")
file(GLOB SOURCES_COLLISION "src/collision/*.cpp")
file(GLOB SOURCES_COMMON "src/common/*.cpp")
file(GLOB SOURCES_DYNAMICS "src/dynamics/*.cpp")
file(GLOB SOURCES_ROPE "src/rope/*.cpp")


add_library(box2d STATIC 
    ${SOURCES} 
    ${SOURCES_COLLISION} 
    ${SOURCES_COMMON} 
    ${SOURCES_DYNAMICS} 
    ${SOURCES_ROPE}
)


target_include_directories(box2d PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


