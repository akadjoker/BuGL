cmake_minimum_required(VERSION 3.10)
project(poly2tri)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti -fno-strict-aliasing ")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)




file(GLOB SOURCES src/common/*.cc src/sweep/*.cc)
file(GLOB HEADERS include/*.h include/common/*.h include/sweep/*.h)


add_library(poly2tri ${SOURCES} ${HEADERS})
target_include_directories(poly2tri INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


target_include_directories(poly2tri PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


