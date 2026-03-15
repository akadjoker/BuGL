project(main)
cmake_policy(SET CMP0072 NEW)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -D_CRT_SECURE_NO_WARNINGS")
    if (MSVC)
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            add_compile_options(/RTC1 /Od /Zi)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        endif()     
    endif()

endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(main   ${SOURCES}
    ${CMAKE_SOURCE_DIR}/main/src/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/main/src/imgui_impl_opengl3.cpp
)
if (WIN32)
    # MinGW may track an import library side output for executables with ENABLE_EXPORTS.
    # Keep it off on Windows to avoid perpetual relink; plugins link libbu directly there.
    set_target_properties(main PROPERTIES ENABLE_EXPORTS OFF)
else()
    # Unix plugins resolve host symbols from the main executable.
    set_target_properties(main PROPERTIES ENABLE_EXPORTS ON)
endif()
set_target_properties(main PROPERTIES OUTPUT_NAME "bugl")
target_include_directories(main PUBLIC include src 
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/main/src
)
if (WIN32)
    target_compile_definitions(main PRIVATE WINGDIAPI=)
    if (NOT MSVC)
        target_compile_options(main PRIVATE -U_DLL)
    endif()
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)

if (UNIX)
target_compile_options(main PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g  -D_DEBUG -DVERBOSE)
target_link_options(main PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g  -D_DEBUG) 
endif()
 
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(main PRIVATE
        -O3
        -DNDEBUG
        -ftree-vectorize
        -finline-functions
        -funroll-loops
    )

    if (NOT BUGL_PORTABLE_RELEASE)
        target_compile_options(main PRIVATE -march=native -mtune=native)
    endif()

    if (BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(main PRIVATE -s)
    endif()
endif()

target_link_libraries(main PRIVATE miniz poly2tri ODE box2d bugl_audio libbu imgui SDL2::SDL2)

if (WIN32)
    target_link_libraries(main PRIVATE opengl32)
else()
    target_link_libraries(main PRIVATE GL)
endif()

if (WIN32 AND TARGET SDL2::SDL2main)
    target_link_libraries(main PRIVATE SDL2::SDL2main)
endif()

if (WIN32)
    if (MSVC)
        target_link_libraries(main PRIVATE Winmm.lib)
    else()
        target_link_libraries(main PRIVATE winmm)
    endif()
endif()

if (MINGW)
    get_filename_component(BUGL_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(BUGL_MINGW_RUNTIME_DLLS
        libstdc++-6.dll
        libwinpthread-1.dll
        libgcc_s_seh-1.dll
        libgcc_s_dw2-1.dll
        libgcc_s_sjlj-1.dll
    )

    foreach(BUGL_RUNTIME_DLL IN LISTS BUGL_MINGW_RUNTIME_DLLS)
        if(EXISTS "${BUGL_MINGW_BIN_DIR}/${BUGL_RUNTIME_DLL}")
            add_custom_command(TARGET main POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${BUGL_MINGW_BIN_DIR}/${BUGL_RUNTIME_DLL}"
                    "$<TARGET_FILE_DIR:main>/${BUGL_RUNTIME_DLL}"
                COMMENT "Copying ${BUGL_RUNTIME_DLL} to output directory"
            )
        endif()
    endforeach()
endif()


if (UNIX)
    target_link_libraries(main PRIVATE m)
endif()
