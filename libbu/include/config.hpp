
#pragma once

#include <cstddef>
#include <cassert>
#include <cfloat>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <cstdio>
#include <cmath>

#if defined(__EMSCRIPTEN__)
#define OS_EMSCRIPTEN
#elif defined(_WIN32)
#define OS_WINDOWS
#elif defined(__ANDROID__)
#define OS_ANDROID
#elif defined(__linux__)
#define OS_LINUX
#elif defined(__APPLE__)
#define OS_MAC
#endif

#if defined(__GNUC__) || defined(__clang__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define FORCE_INLINE inline
#define UNLIKELY(x) (x)
#define LIKELY(x) (x)
#endif

// VM dispatch mode:
// 1 = computed goto (faster on GCC/Clang)
// 0 = switch dispatch (portable fallback)
#ifndef USE_COMPUTED_GOTO
#define USE_COMPUTED_GOTO 1
#endif

#if (USE_COMPUTED_GOTO != 0) && (USE_COMPUTED_GOTO != 1)
#error "USE_COMPUTED_GOTO must be 0 or 1"
#endif

#define BU_ENABLE_SOCKETS 1
#define BU_ENABLE_FILE_IO 1
#define BU_ENABLE_MATH 1
#define BU_ENABLE_TIME 1
#define BU_ENABLE_PATH 1
#define BU_ENABLE_OS 1
#define BU_ENABLE_JSON 1
#define BU_ENABLE_REGEX 1
#define BU_ENABLE_ZIP 1
#define BU_ENABLE_TIME 1

#ifndef BU_ENABLE_BYTECODE_DUMP
#if defined(OS_LINUX) || defined(OS_WINDOWS)
#define BU_ENABLE_BYTECODE_DUMP 1
#else
#define BU_ENABLE_BYTECODE_DUMP 0
#endif
#endif

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef float float32;
typedef double float64;

const float32 maxFloat = FLT_MAX;
const float32 epsilon = FLT_EPSILON;
const float32 pi = 3.14159265359f;

template <typename T>
inline T Max(T a, T b)
{
    return a > b ? a : b;
}

#if defined(OS_LINUX)

#define CONSOLE_COLOR_RESET "\033[0m"
#define CONSOLE_COLOR_GREEN "\033[1;32m"
#define CONSOLE_COLOR_RED "\033[1;31m"
#define CONSOLE_COLOR_PURPLE "\033[1;35m"
#define CONSOLE_COLOR_CYAN "\033[0;36m"
#define CONSOLE_COLOR_YELLOW "\033[1;33m"
#define CONSOLE_COLOR_BLUE "\033[0;34m"

#else

#define CONSOLE_COLOR_RESET ""
#define CONSOLE_COLOR_GREEN ""
#define CONSOLE_COLOR_RED ""
#define CONSOLE_COLOR_PURPLE ""
#define CONSOLE_COLOR_CYAN ""
#define CONSOLE_COLOR_YELLOW ""
#define CONSOLE_COLOR_BLUE ""

#endif

void Warning(const char *fmt, ...);
void Info(const char *fmt, ...);
void Error(const char *fmt, ...);
void Print(const char *fmt, ...);
void Trace(int severity, const char *fmt, ...);

#define INFO(fmt, ...) Log(0, fmt, ##__VA_ARGS__)
#define WARNING(fmt, ...) Log(1, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) Log(2, fmt, ##__VA_ARGS__)
#define PRINT(fmt, ...) Log(3, fmt, ##__VA_ARGS__)

#if defined(_DEBUG)
#include <assert.h>
#define DEBUG_BREAK_IF(condition)                                          \
    if (condition)                                                         \
    {                                                                      \
        Error("Debug break: %s at %s:%d", #condition, __FILE__, __LINE__); \
        std::exit(EXIT_FAILURE);                                           \
    }
#else
#define DEBUG_BREAK_IF(_CONDITION_)
#endif

inline size_t CalculateCapacityGrow(size_t capacity, size_t minCapacity)
{
    if (capacity < minCapacity)
        capacity = minCapacity;
    if (capacity < 8)
    {
        capacity = 8;
    }
    else
    {
        // Round up to the next power of 2 and multiply by 2 (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
        capacity--;
        capacity |= capacity >> 1;
        capacity |= capacity >> 2;
        capacity |= capacity >> 4;
        capacity |= capacity >> 8;
        capacity |= capacity >> 16;
        capacity++;
    }
    return capacity;
}

static inline size_t GROW_CAPACITY(size_t capacity)
{
    return ((capacity) < 8 ? 8 : (capacity) * 2);
}

static inline void *aAlloc(size_t size)
{
    return std::malloc(size);
}

static inline void *aRealloc(void *buffer, size_t size)
{
    return std::realloc(buffer, size);
}

static inline void aFree(void *mem)
{
    std::free(mem);
}
