project(imgui)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/modules)

# ImGui source files (copy imgui sources to src/ or use external/)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
if (NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_stdlib.cpp
    #${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    #${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

if (BUGL_IMGUI_DEMO)
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/imgui_demo.cpp)
endif()

# Source files
set(BINDING_SOURCES
    ${IMGUI_SOURCES}
)

# Find dependencies
# find_package(OpenGL REQUIRED)
# find_package(glfw3 REQUIRED)

 
add_library(imgui STATIC  ${BINDING_SOURCES})

target_include_directories(imgui PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${IMGUI_DIR}
    #${IMGUI_DIR}/backends
)

if (BUGL_IMGUI_DEMO)
    target_compile_definitions(imgui PUBLIC)
else()
    target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_DEMO_WINDOWS)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if (UNIX)
        target_compile_options(imgui PRIVATE -g -D_DEBUG -DVERBOSE -fPIC)
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(imgui PRIVATE -O3 -DNDEBUG -fPIC)
endif()

if (UNIX)
    set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set_target_properties(imgui PROPERTIES
    OUTPUT_NAME "imgui"
    PREFIX "lib"
)
