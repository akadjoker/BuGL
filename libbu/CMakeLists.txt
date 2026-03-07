cmake_minimum_required(VERSION 3.20)

project(bu)
set(CMAKE_CXX_STANDARD 11)
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
add_library(libbu STATIC ${SOURCES})

# Enable PIC so libbu can be linked into shared libraries (plugins)
set_target_properties(libbu PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(libbu PUBLIC include  src)

# ============================================
# Compiler Flags - DEBUG
# ============================================
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "🐛 Debug mode - sanitizers enabled")
    
    # target_compile_options(libbu PRIVATE
    #     # Sanitizers
    #     -fsanitize=address
    #     -fsanitize=undefined,integer,float-divide-by-zero,unsigned-integer-overflow
    #     -fsanitize=leak
        
    #     # Debug info
    #     -g
    #     -O0
        
    #     # Warnings
    #     -Wall
    #     -Wextra
    #     -Wpedantic
        
    #     # Debug defines
    #     -DDEBUG
    #     -D_DEBUG
    # )
    
    # target_link_options(libbu PRIVATE
    #     -fsanitize=address
    #     -fsanitize=undefined,integer,float-divide-by-zero,unsigned-integer-overflow
    #     -fsanitize=leak
    # )
 
    target_compile_options(libbu PRIVATE
        -O2
        -g
        -fno-omit-frame-pointer
    )
 

# ============================================
# Compiler Flags - RELEASE
# ============================================
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "🚀 Release mode - full optimizations")
    
    target_compile_options(libbu PRIVATE
        # Optimization level
        -O3
        
    
        
        # Architecture specific
        -march=native
        -mtune=native
        
      
        
        # Vectorization
        -ftree-vectorize
        
        # Strip debug info
        -DNDEBUG
        
        # Inline agressivo
        -finline-functions
        -funroll-loops
    )
    
    target_link_options(libbu PRIVATE

        -s  # Strip symbols
    )
    
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
        -march=native
    )
endif()
 
# ============================================
# Platform Specific
# ============================================
if(UNIX AND NOT APPLE)
    target_link_libraries(libbu PRIVATE miniz pthread)
endif()

# ============================================
# Info
# ============================================
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
