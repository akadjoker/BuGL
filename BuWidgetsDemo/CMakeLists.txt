project(BuWidgetsDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUWIDGETS_DEMO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/BuEditor/src/ImGuiFileDialog.cpp
    ${CMAKE_SOURCE_DIR}/BuEditor/src/ImGuiConsole.cpp
    ${CMAKE_SOURCE_DIR}/BuEditor/src/ImGuiPopup.cpp
    ${CMAKE_SOURCE_DIR}/BuEditor/src/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/BuEditor/src/imgui_impl_opengl3.cpp
)

if(WIN32)
    list(APPEND BUWIDGETS_DEMO_SOURCES ${CMAKE_SOURCE_DIR}/main/src/opengl_loader_windows.cpp)
endif()

add_executable(BuWidgetsDemo ${BUWIDGETS_DEMO_SOURCES})
set_target_properties(BuWidgetsDemo PROPERTIES OUTPUT_NAME "buwidgets_demo")

target_include_directories(BuWidgetsDemo PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/BuEditor/src
)

if(WIN32)
    target_compile_definitions(BuWidgetsDemo PRIVATE WINGDIAPI=)
    if(NOT MSVC)
        target_compile_options(BuWidgetsDemo PRIVATE -U_DLL)
    endif()
endif()

target_link_libraries(BuWidgetsDemo PRIVATE imgui SDL2::SDL2-static)

if(WIN32)
    target_link_libraries(BuWidgetsDemo PRIVATE opengl32)

    if(TARGET SDL2::SDL2main)
        target_link_libraries(BuWidgetsDemo PRIVATE SDL2::SDL2main)
    endif()

    if(MSVC)
        target_link_libraries(BuWidgetsDemo PRIVATE Winmm.lib)
    else()
        target_link_libraries(BuWidgetsDemo PRIVATE winmm)
    endif()
else()
    target_link_libraries(BuWidgetsDemo PRIVATE GL m)
endif()
