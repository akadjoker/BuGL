cmake_minimum_required(VERSION 3.20)

project(bulang_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

function(bugl_configure_test_target target_name)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/libbu/include
        ${CMAKE_SOURCE_DIR}/libbu/src
    )

    target_compile_definitions(${target_name} PRIVATE
        BULANG_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )

    target_link_libraries(${target_name} PRIVATE libbu)

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        if(UNIX)
            target_compile_options(${target_name} PRIVATE
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
                -g
                -O1
                -fno-omit-frame-pointer
                -D_DEBUG
                -DVERBOSE
            )

            target_link_options(${target_name} PRIVATE
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
                -g
            )
        else()
            target_compile_options(${target_name} PRIVATE
                -g
                -O1
                -fno-omit-frame-pointer
            )
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES Release)
        target_compile_options(${target_name} PRIVATE
            -O3
            -DNDEBUG
        )

        if(BUGL_STRIP_RELEASE AND NOT MSVC)
            target_link_options(${target_name} PRIVATE -s)
        endif()
    endif()

    if(UNIX)
        target_link_libraries(${target_name} PRIVATE m)
    endif()
endfunction()

add_executable(bulang-tests
    src/main.cpp
)

set_target_properties(bulang-tests PROPERTIES
    OUTPUT_NAME "bulang-tests"
)

bugl_configure_test_target(bulang-tests)

file(GLOB TEST_PLUGIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp"
)

set(BULANG_TEST_PLUGIN_TARGETS "")

foreach(plugin_source IN LISTS TEST_PLUGIN_SOURCES)
    get_filename_component(plugin_name "${plugin_source}" NAME_WE)
    set(plugin_target "bulang-plugin-${plugin_name}")

    add_library(${plugin_target} MODULE
        ${plugin_source}
    )

    set_target_properties(${plugin_target} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "libbu_${plugin_name}"
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
    )

    bugl_configure_test_target(${plugin_target})
    list(APPEND BULANG_TEST_PLUGIN_TARGETS ${plugin_target})
endforeach()

if(BULANG_TEST_PLUGIN_TARGETS)
    add_dependencies(bulang-tests ${BULANG_TEST_PLUGIN_TARGETS})
endif()

add_test(NAME bulang.core COMMAND bulang-tests)
set_tests_properties(bulang.core PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
