cmake_minimum_required(VERSION 3.20)

project(bulang_bench)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(bulang-bench
    src/main.cpp
)

set_target_properties(bulang-bench PROPERTIES
    OUTPUT_NAME "bulang-bench"
)

target_include_directories(bulang-bench PRIVATE
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${CMAKE_SOURCE_DIR}/libbu/src
)

target_compile_definitions(bulang-bench PRIVATE
    BULANG_BENCH_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(bulang-bench PRIVATE libbu)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(UNIX)
        target_compile_options(bulang-bench PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
            -O1
            -fno-omit-frame-pointer
            -D_DEBUG
        )

        target_link_options(bulang-bench PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
        )
    else()
        target_compile_options(bulang-bench PRIVATE
            -g
            -O1
            -fno-omit-frame-pointer
        )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bulang-bench PRIVATE
        -O3
        -DNDEBUG
    )

    if(BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bulang-bench PRIVATE -s)
    endif()
endif()

if(UNIX)
    target_link_libraries(bulang-bench PRIVATE m)
endif()
