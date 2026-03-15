project(BuMicroPather)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)

set(BINDING_SOURCES
    src/micropather_core.cpp
    src/micropather_grid.cpp
    src/micropather_bindings.cpp
)

add_library(bu_micropather SHARED
    src/plugin.cpp
    ${BINDING_SOURCES}
    ${CMAKE_SOURCE_DIR}/vendor/MicroPather/micropather.cpp
)

target_include_directories(bu_micropather PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${CMAKE_SOURCE_DIR}/vendor/MicroPather
)

target_compile_definitions(bu_micropather PRIVATE BU_PLUGIN_BUILD)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(UNIX)
        target_compile_options(bu_micropather PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
            -O1
            -fno-omit-frame-pointer
            -D_DEBUG
            -DVERBOSE
            -fPIC
        )
        target_link_options(bu_micropather PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
        )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bu_micropather PRIVATE -O3 -DNDEBUG -fPIC)
    if(BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bu_micropather PRIVATE -s)
    endif()
endif()

if(UNIX)
    set_target_properties(bu_micropather PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set_target_properties(bu_micropather PROPERTIES
    OUTPUT_NAME "bu_micropather"
    PREFIX "lib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
)

if(WIN32)
    target_link_libraries(bu_micropather PRIVATE libbu)
else()
    target_link_libraries(bu_micropather PRIVATE libbu m dl)
endif()
