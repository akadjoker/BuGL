cmake_minimum_required(VERSION 3.20)

project(bu)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================
# Output Directory
# ============================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# ============================================
# Sources
# ============================================
file(GLOB SOURCES "src/*.cpp")
 
# ============================================
# Targets
# ============================================
option(BU_BUILD_SHARED_LIBS "Build libbu as a shared library" OFF)

set(BU_LIBRARY_TYPE STATIC)
if(BU_BUILD_SHARED_LIBS)
    set(BU_LIBRARY_TYPE SHARED)
endif()

add_library(libbu ${BU_LIBRARY_TYPE} ${SOURCES})

set(BU_VERSION_STRING "${CMAKE_PROJECT_VERSION}")
if(NOT BU_VERSION_STRING)
    set(BU_VERSION_STRING "${PROJECT_VERSION}")
endif()
if(NOT BU_VERSION_STRING)
    set(BU_VERSION_STRING "0.0.0")
endif()

set(BU_VERSION_GIT "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD
        OUTPUT_VARIABLE BU_VERSION_GIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT BU_VERSION_GIT)
        set(BU_VERSION_GIT "unknown")
    endif()
endif()

target_compile_definitions(libbu PUBLIC
    BU_VERSION_STRING=\"${BU_VERSION_STRING}\"
    BU_VERSION_GIT=\"${BU_VERSION_GIT}\"
)

# Build libbu as a static library by default, with an opt-in shared build.
set_target_properties(libbu PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    DEBUG_POSTFIX ""
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

if(WIN32 AND BU_BUILD_SHARED_LIBS)
    set_target_properties(libbu PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(MINGW AND BU_BUILD_SHARED_LIBS)
    target_link_options(libbu PRIVATE -shared-libgcc)
endif()

target_include_directories(libbu PUBLIC 
    include 
    src 
    ${CMAKE_SOURCE_DIR}/vendor/miniz/include
    ${CMAKE_SOURCE_DIR}/vendor/eigen
    ${CMAKE_SOURCE_DIR}/vendor/MiniDNN/include
)

# ============================================
# Compiler Flags - DEBUG
# ============================================
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "🐛 Debug mode - sanitizers enabled")

    if (UNIX)
        target_compile_options(libbu PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
            -O1
            -fno-omit-frame-pointer
            -D_DEBUG
            -DVERBOSE
        )

        target_link_options(libbu PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
        )
    else()
        target_compile_options(libbu PRIVATE
            -g
            -O1
            -fno-omit-frame-pointer
        )
    endif()
 

# ============================================
# Compiler Flags - RELEASE
# ============================================
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "🚀 Release mode - full optimizations")
    
    target_compile_options(libbu PRIVATE
        # Optimization level
        -O3
        
      
        
        # Vectorization
        -ftree-vectorize
        
        # Strip debug info
        -DNDEBUG
        
        # Inline agressivo
        -finline-functions
        -funroll-loops
    )

    if (NOT BUGL_PORTABLE_RELEASE)
        target_compile_options(libbu PRIVATE
            -march=native
            -mtune=native
        )
    endif()
    
    if (BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(libbu PRIVATE -s)
    endif()
    
# ============================================
# Compiler Flags - RELWITHDEBINFO
# ============================================
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message(STATUS "🔧 RelWithDebInfo mode - optimized + debug symbols")
    
    target_compile_options(libbu PRIVATE
        -O2
        -g
        -march=native
    )
    
# ============================================
# Default to Release
# ============================================
else()
    message(STATUS "⚠️  No build type specified, defaulting to Release")
    set(CMAKE_BUILD_TYPE Release)
    
    target_compile_options(libbu PRIVATE
        -O3
    )

    if (NOT BUGL_PORTABLE_RELEASE)
        target_compile_options(libbu PRIVATE -march=native -mtune=native)
    endif()
endif()
 
# ============================================
# Platform Specific
# ============================================
if(WIN32)
    target_link_libraries(libbu PRIVATE miniz ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(libbu PRIVATE miniz pthread)
endif()

# ============================================
# Info
# ============================================
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "libbu type: ${BU_LIBRARY_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
