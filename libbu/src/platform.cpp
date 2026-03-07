#include "platform.hpp"

#include <cstdarg>

// Dynamic library loading headers
#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

// ============================================
// LINUX/DESKTOP Platform
// ============================================
#ifdef __linux__

void OsPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}

void OsEPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

#endif // __linux__

// ============================================
// WINDOWS Platform
// ============================================
#ifdef _WIN32

void OsPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}

void OsEPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

#endif // _WIN32

// ============================================
// MAC/APPLE Platform
// ============================================
#ifdef __APPLE__

void OsPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}

void OsEPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

#endif // __APPLE__

// ============================================
// WASM/EMSCRIPTEN Platform (WEB/PLAYGROUND)
// ============================================
#ifdef __EMSCRIPTEN__

#include <string>
#include <emscripten.h>
#include "Outputcapture.h"

OutputCapture *g_currentOutput = nullptr;

void OsPrintf(const char *fmt, ...)
{
    char buffer[4096];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len > 0)
    {
        if (g_currentOutput)
        {
            //  Playground mode - capture to OutputCapture
            g_currentOutput->write(std::string(buffer, len));
        }
        else
        {
            //  Fallback - direct printf
            printf("%s", buffer);
        }
    }
}

void OsEPrintf(const char *fmt, ...)
{
    char buffer[4096];

    va_list args;
    va_start(args, fmt);

    // Add error prefix
    int offset = snprintf(buffer, sizeof(buffer), "❌ ERROR: ");
    if (offset < 0)
        offset = 0;

    int len = vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, args);
    va_end(args);

    if (len > 0)
    {
        if (g_currentOutput)
        {

            g_currentOutput->write(std::string(buffer, offset + len));
        }
        else
        {
            //  Fallback
            printf("%s", buffer);
        }
    }
}

// Emscripten tem filesystem virtual via IDBFS
int OsFileWrite(const char *filename, const void *data, size_t size)
{
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        OsEPrintf("Failed to open file '%s' for writing", filename);
        return -1;
    }

    size_t written = fwrite(data, 1, size, file);
    fclose(file);

    // Sync filesystem
    EM_ASM(
        FS.syncfs(false, function(err) {
            if (err) console.error('FS sync error:', err); }););

    if (written != size)
    {
        return -1;
    }

    return (int)written;
}

int OsFileRead(const char *filename, void *buffer, size_t maxSize)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        OsEPrintf("Failed to open file '%s' for reading", filename);
        return -1;
    }

    if (buffer == NULL)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fclose(file);
        return (int)size;
    }

    size_t read = fread(buffer, 1, maxSize, file);
    fclose(file);

    return (int)read;
}

bool OsFileExists(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file)
    {
        fclose(file);
        return true;
    }
    return false;
}

int OsFileSize(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return -1;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);

    return (int)size;
}

bool OsFileDelete(const char *filename)
{
    bool result = remove(filename) == 0;

    // Sync filesystem
    EM_ASM(
        FS.syncfs(false, function(err) {
            if (err) console.error('FS sync error:', err); }););

    return result;
}

#endif // __EMSCRIPTEN__

// ============================================
// LINUX/DESKTOP/WINDOWS/MAC
// ============================================
#if defined(__linux__) || defined(_WIN32) || defined(__APPLE__)

int OsFileWrite(const char *filename, const void *data, size_t size)
{
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        OsEPrintf("Failed to open file '%s' for writing", filename);
        return -1;
    }

    size_t written = fwrite(data, 1, size, file);
    fclose(file);

    if (written != size)
    {
        OsEPrintf("Failed to write %zu bytes (wrote %zu)", size, written);
        return -1;
    }

    return (int)written;
}

int OsFileRead(const char *filename, void *buffer, size_t maxSize)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        OsEPrintf("Failed to open file '%s' for reading", filename);
        return -1;
    }

 
    if (buffer == NULL)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fclose(file);
        return (int)size;
    }

    size_t read = fread(buffer, 1, maxSize, file);
    fclose(file);

    return (int)read;
}

bool OsFileExists(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file)
    {
        fclose(file);
        return true;
    }
    return false;
}

int OsFileSize(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return -1;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);

    return (int)size;
}

bool OsFileDelete(const char *filename)
{
    return remove(filename) == 0;
}

#endif

// ============================================
// Dynamic Library Loading
// ============================================

#if defined(__linux__) || defined(__APPLE__)

void* OsLoadLibrary(const char* path)
{
    // RTLD_GLOBAL: symbols are available to subsequently loaded plugins
    // This allows bu_rlgl to find glfwGetProcAddress from bu_glfw
    void* handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    return handle;
}

void* OsGetSymbol(void* handle, const char* symbol)
{
    if (!handle) return nullptr;
    return dlsym(handle, symbol);
}

void OsFreeLibrary(void* handle)
{
    if (handle) {
        dlclose(handle);
    }
}

const char* OsGetLibraryError()
{
    return dlerror();
}

const char* OsGetLibraryExtension()
{
#if defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

#elif defined(_WIN32)

void* OsLoadLibrary(const char* path)
{
    return (void*)LoadLibraryA(path);
}

void* OsGetSymbol(void* handle, const char* symbol)
{
    if (!handle) return nullptr;
    return (void*)GetProcAddress((HMODULE)handle, symbol);
}

void OsFreeLibrary(void* handle)
{
    if (handle) {
        FreeLibrary((HMODULE)handle);
    }
}

static char s_winErrorBuffer[256];

const char* OsGetLibraryError()
{
    DWORD error = GetLastError();
    if (error == 0) return nullptr;

    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        s_winErrorBuffer,
        sizeof(s_winErrorBuffer),
        nullptr
    );
    return s_winErrorBuffer;
}

const char* OsGetLibraryExtension()
{
    return ".dll";
}

#elif defined(__EMSCRIPTEN__)

// WASM doesn't support dynamic loading
void* OsLoadLibrary(const char* path)
{
    (void)path;
    return nullptr;
}

void* OsGetSymbol(void* handle, const char* symbol)
{
    (void)handle;
    (void)symbol;
    return nullptr;
}

void OsFreeLibrary(void* handle)
{
    (void)handle;
}

const char* OsGetLibraryError()
{
    return "Dynamic library loading not supported on WASM";
}

const char* OsGetLibraryExtension()
{
    return ".wasm";
}

#endif
