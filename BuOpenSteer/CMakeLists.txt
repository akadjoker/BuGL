project(BuOpenSteer)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    if (MSVC)
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            add_compile_options(/RTC1 /Od /Zi)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        endif()
    endif()
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)

set(BINDING_SOURCES
    src/opensteer_core.cpp
    src/opensteer_stubs.cpp
    src/opensteer_bindings.cpp
    src/opensteer_agent.cpp
    src/opensteer_obstacle.cpp
)

add_library(bu_opensteer SHARED src/plugin.cpp ${BINDING_SOURCES})

target_include_directories(bu_opensteer PRIVATE
    include
    src
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${CMAKE_SOURCE_DIR}/main/src
    ${CMAKE_SOURCE_DIR}/vendor/OpenSteer/include
)

target_compile_definitions(bu_opensteer PRIVATE BU_PLUGIN_BUILD)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if (UNIX)
        target_compile_options(bu_opensteer PRIVATE
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
        target_link_options(bu_opensteer PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
        )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bu_opensteer PRIVATE -O3 -DNDEBUG -fPIC)
    if (BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bu_opensteer PRIVATE -s)
    endif()
endif()

if (UNIX)
    set_target_properties(bu_opensteer PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set_target_properties(bu_opensteer PROPERTIES
    OUTPUT_NAME "bu_opensteer"
    PREFIX "lib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
)

if (WIN32)
    if (MSVC)
        target_link_libraries(bu_opensteer PRIVATE libopensteer libbu Winmm.lib)
    else()
        target_link_libraries(bu_opensteer PRIVATE libopensteer libbu winmm)
    endif()
else()
    target_link_libraries(bu_opensteer PRIVATE libopensteer m dl)
endif()
