project(BuEditor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(BUEDITOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DocumentSymbols.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GifRecorder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGuiFileDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGuiConsole.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TextEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LanguageDefinitions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_sdl2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/regex/src/regex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/regex/src/static_mutex.cpp
)

if(WIN32)
    # Keep MinGW compatibility for OpenGL symbol imports.
    list(APPEND BUEDITOR_SOURCES ${CMAKE_SOURCE_DIR}/main/src/opengl_loader_windows.cpp)
endif()

add_executable(BuEditor ${BUEDITOR_SOURCES})
set_target_properties(BuEditor PROPERTIES OUTPUT_NAME "bueditor")

if(NOT TARGET SDL2::SDL2-static)
    message(FATAL_ERROR "BuEditor requires vendored SDL2 from vendor/SDL")
endif()

foreach(BUEDITOR_RUNTIME_TARGET IN ITEMS
    main
    bu_assimp
    bu_box2d
    bu_chip2d
    bu_jolt
    bu_micropather
    bu_ode
    bu_opensteer
    bu_recast
)
    if(TARGET ${BUEDITOR_RUNTIME_TARGET})
        add_dependencies(BuEditor ${BUEDITOR_RUNTIME_TARGET})
    endif()
endforeach()

target_include_directories(BuEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/main/src
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/assimp/contrib/rapidjson/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/regex/include
)

target_compile_definitions(BuEditor PRIVATE BOOST_REGEX_STANDALONE BOOST_REGEX_NO_LIB)

if(WIN32)
    target_compile_definitions(BuEditor PRIVATE WINGDIAPI=)
    if(NOT MSVC)
        target_compile_options(BuEditor PRIVATE -U_DLL)
    endif()
endif()

target_link_libraries(BuEditor PRIVATE imgui SDL2::SDL2-static)

if(WIN32)
    target_link_libraries(BuEditor PRIVATE opengl32)

    if(TARGET SDL2::SDL2main)
        target_link_libraries(BuEditor PRIVATE SDL2::SDL2main)
    endif()

    if(MSVC)
        target_link_libraries(BuEditor PRIVATE Winmm.lib)
    else()
        target_link_libraries(BuEditor PRIVATE winmm)
    endif()
else()
    target_link_libraries(BuEditor PRIVATE GL m)
endif()
