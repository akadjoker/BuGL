project(BuDebugHost)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(BuDebugHost
    src/main.cpp
)

set_target_properties(BuDebugHost PROPERTIES
    OUTPUT_NAME "budebughost"
)

if(WIN32)
    set_target_properties(BuDebugHost PROPERTIES ENABLE_EXPORTS OFF)
else()
    set_target_properties(BuDebugHost PROPERTIES ENABLE_EXPORTS ON)
endif()

target_include_directories(BuDebugHost PRIVATE
    ${CMAKE_SOURCE_DIR}/libbu/include
)

target_link_libraries(BuDebugHost PRIVATE libbu)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(UNIX)
        target_compile_options(BuDebugHost PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g -D_DEBUG -DVERBOSE)
        target_link_options(BuDebugHost PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g -D_DEBUG)
    else()
        target_compile_options(BuDebugHost PRIVATE -g -O1 -fno-omit-frame-pointer)
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(BuDebugHost PRIVATE -O3 -DNDEBUG)
    if(NOT BUGL_PORTABLE_RELEASE)
        target_compile_options(BuDebugHost PRIVATE -march=native -mtune=native)
    endif()
    if(BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(BuDebugHost PRIVATE -s)
    endif()
endif()

if(UNIX)
    target_link_libraries(BuDebugHost PRIVATE m)
endif()
