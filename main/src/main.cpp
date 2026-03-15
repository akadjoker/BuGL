

 
#include "config.hpp"
#include "version.h"
#include "filebuffer.hpp"
#include "interpreter.hpp"
#include "bindings.hpp"
#include "platform.hpp"
#include "utils.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <exception>
#include <algorithm>
#include <filesystem>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
 
 extern "C" const char *__lsan_default_suppressions()
{
    return "leak:libSDL2\n"
           "leak:SDL_DBus\n";
}

struct FileLoaderContext
{
    const char *searchPaths[8];
    int pathCount;
    char fullPath[512];
    FileBuffer fileBuffer;
    std::vector<unsigned char> rawFileData;
};

static bool isAbsolutePath(const char *path)
{
    if (!path || !*path)
        return false;
    if (path[0] == '/' || path[0] == '\\')
        return true;
#if defined(_WIN32)
    if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) &&
        path[1] == ':')
    {
        return true;
    }
#endif
    return false;
}

static void pathDirname(const char *path, char *out, size_t outSize)
{
    if (!out || outSize == 0)
        return;
    out[0] = '\0';

    if (!path || !*path)
    {
        snprintf(out, outSize, ".");
        return;
    }

    const char *slash1 = strrchr(path, '/');
    const char *slash2 = strrchr(path, '\\');
    const char *slash = slash1;
    if (slash2 && (!slash1 || slash2 > slash1))
        slash = slash2;

    if (!slash)
    {
        snprintf(out, outSize, ".");
        return;
    }

    size_t len = (size_t)(slash - path);
    if (len == 0)
    {
        snprintf(out, outSize, "/");
        return;
    }

    if (len >= outSize)
        len = outSize - 1;
    memcpy(out, path, len);
    out[len] = '\0';
}

static bool hasSuffix(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return false;
    size_t strLen = std::strlen(str);
    size_t suffixLen = std::strlen(suffix);
    if (suffixLen > strLen)
        return false;
    return std::strcmp(str + (strLen - suffixLen), suffix) == 0;
}

static bool isBytecodePath(const char *path)
{
    return hasSuffix(path, ".buc") || hasSuffix(path, ".bubc") || hasSuffix(path, ".bytecode");
}

static std::string getExecutablePath(const char *argv0)
{
#if defined(_WIN32)
    char buffer[1024] = {0};
    DWORD len = GetModuleFileNameA(nullptr, buffer, (DWORD)sizeof(buffer));
    if (len > 0 && len < sizeof(buffer))
        return std::filesystem::path(buffer).lexically_normal().string();
#elif defined(__linux__)
    char buffer[1024] = {0};
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len > 0)
    {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).lexically_normal().string();
    }
#elif defined(__APPLE__)
    char buffer[1024] = {0};
    uint32_t size = (uint32_t)sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
        return std::filesystem::path(buffer).lexically_normal().string();
#endif

    if (argv0 && *argv0)
    {
        std::error_code ec;
        std::filesystem::path p = std::filesystem::absolute(argv0, ec);
        if (!ec)
            return p.lexically_normal().string();
    }

    return std::filesystem::current_path().string();
}

static void addUniqueVmPluginSearchPath(Interpreter &vm, std::vector<std::string> &paths, const std::filesystem::path &path)
{
    if (path.empty())
        return;

    const std::string normalized = path.lexically_normal().string();
    if (normalized.empty())
        return;

    for (const std::string &existing : paths)
    {
        if (existing == normalized)
            return;
    }

    paths.push_back(normalized);
    vm.addPluginSearchPath(paths.back().c_str());
}

const char *multiPathFileLoader(const char *filename, size_t *outSize, void *userdata)
{
    if (!filename || !outSize || !userdata)
        return nullptr;

    FileLoaderContext *ctx = (FileLoaderContext *)userdata;
    *outSize = 0;

    auto tryLoadPath = [&](const char *path) -> const char *
    {
        if (!path || !*path)
            return nullptr;

        if (ctx->fileBuffer.load(path))
        {
            ctx->rawFileData.clear();
            *outSize = ctx->fileBuffer.size();
            return ctx->fileBuffer.c_str();
        }

        unsigned int bytesRead = 0;
        unsigned char *raw = Utils::LoadFileData(path, &bytesRead);
        if (raw && bytesRead > 0)
        {
            ctx->rawFileData.assign(raw, raw + bytesRead);
            Utils::UnloadFileData(raw);
            *outSize = ctx->rawFileData.size();
            return reinterpret_cast<const char *>(ctx->rawFileData.data());
        }

        if (raw)
            Utils::UnloadFileData(raw);
        return nullptr;
    };

    // If include path starts with '/' (ex: "/scripts/main.bu"), also keep relative form.
    const char *relativeName = filename;
    while (*relativeName == '/' || *relativeName == '\\')
    {
        relativeName++;
    }

    // Absolute filesystem path: try directly.
    if (isAbsolutePath(filename))
    {
        return tryLoadPath(filename);
    }

    // Relative include: search configured script roots first.
    const char *namePart = (relativeName != filename) ? relativeName : filename;
    for (int i = 0; i < ctx->pathCount; i++)
    {
        snprintf(ctx->fullPath, sizeof(ctx->fullPath), "%s/%s", ctx->searchPaths[i], namePart);
        const char *loaded = tryLoadPath(ctx->fullPath);
        if (loaded)
            return loaded;
    }

    // Last fallback: current working directory relative.
    const char *loaded = tryLoadPath(filename);
    if (loaded)
        return loaded;
    if (relativeName != filename)
        return tryLoadPath(relativeName);

    return nullptr;
}

// Helper: load file contents into a string
static std::string loadFile(const char *path, bool quiet = false)
{
    FileBuffer file;
    if (!file.load(path))
    {
        if (!quiet)
        {
           LogWarning( "Could not open file: %s", path ? path : "<null>");
        }
        return "";
    }
    return file.toString();
}


void showFatalScreen(const std::string &message)
{
    
}
 

int main(int argc, char *argv[])
{
    std::printf("BuGL version %s (git %s)\n", bugl::version::string(), bugl::version::git());

    Interpreter vm;
    vm.registerAll();
    SDLBindings::registerAll(vm);

    std::vector<std::string> pluginSearchPaths;
    const std::filesystem::path exePath = getExecutablePath(argc > 0 ? argv[0] : nullptr);
    const std::filesystem::path exeDir = exePath.has_parent_path() ? exePath.parent_path() : std::filesystem::current_path();
    const std::filesystem::path cwd = std::filesystem::current_path();

    // Release layout: <bugl>/plugins. Development layout: <repo>/bin/plugins (or bin/modules).
    addUniqueVmPluginSearchPath(vm, pluginSearchPaths, exeDir / "plugins");
    addUniqueVmPluginSearchPath(vm, pluginSearchPaths, exeDir / "modules");
    addUniqueVmPluginSearchPath(vm, pluginSearchPaths, cwd / "bin" / "plugins");
    addUniqueVmPluginSearchPath(vm, pluginSearchPaths, cwd / "bin" / "modules");
    addUniqueVmPluginSearchPath(vm, pluginSearchPaths, cwd / "plugins");
    addUniqueVmPluginSearchPath(vm, pluginSearchPaths, cwd / "modules");

    FileLoaderContext ctx{};

    enum class LaunchMode
    {
        RunSource,
        RunBytecode,
        CompileBytecode
    };

#ifdef BU_RUNNER_ONLY
    LaunchMode mode = LaunchMode::RunBytecode;
#else
    LaunchMode mode = LaunchMode::RunSource;
#endif
    const char *scriptFile = nullptr;
    const char *bytecodeOutFile = nullptr;
    const char *dumpOutputFile = "main.dump";
    bool dumpToFile = false;
    std::string code;
    std::string resolvedScriptFile;

    for (int argi = 1; argi < argc; ++argi)
    {
        const char *arg = argv[argi];
        if (!arg)
            continue;

        if (std::strcmp(arg, "--dump") == 0 || std::strcmp(arg, "--dump-bytecode") == 0)
        {
            dumpToFile = true;
            continue;
        }

        if (std::strcmp(arg, "--dump-file") == 0)
        {
            if (argi + 1 >= argc)
            {
                LogError("Usage: --dump-file <output.dump>");
                return 1;
            }

            dumpToFile = true;
            dumpOutputFile = argv[++argi];
            continue;
        }

#ifdef BU_RUNNER_ONLY
        if (std::strcmp(arg, "--compile-bc") == 0 || std::strcmp(arg, "--compile-bytecode") == 0)
        {
            std::string msg = "Runner build is bytecode-only. Use desktop main to compile .buc files.";
            LogError("%s", msg.c_str());
            return 1;
        }
#endif

        if (std::strcmp(arg, "--compile-bc") == 0 || std::strcmp(arg, "--compile-bytecode") == 0)
        {
            mode = LaunchMode::CompileBytecode;
            if (argi + 2 >= argc)
            {
                std::string msg = "Usage: main --compile-bc <input.bu> <output.buc> [--dump] [--dump-file file]";
                LogError("%s", msg.c_str());
                return 1;
            }
            scriptFile = argv[++argi];
            bytecodeOutFile = argv[++argi];
            continue;
        }

        if (std::strcmp(arg, "--run-bc") == 0 || std::strcmp(arg, "--run-bytecode") == 0)
        {
            mode = LaunchMode::RunBytecode;
            if (argi + 1 >= argc)
            {
                std::string msg = "Usage: main --run-bc <file.buc> [--dump] [--dump-file file]";
                LogError("%s", msg.c_str());
                return 1;
            }
            scriptFile = argv[++argi];
            continue;
        }

        if (arg[0] == '-')
        {
            LogError("Unknown option: %s", arg);
            return 1;
        }

        if (scriptFile != nullptr)
        {
            LogError("Unexpected extra argument: %s", arg);
            return 1;
        }

        scriptFile = arg;
        if (isBytecodePath(scriptFile))
        {
            mode = LaunchMode::RunBytecode;
        }
#ifdef BU_RUNNER_ONLY
        else
        {
            std::string msg = "Runner expects a bytecode file (.buc/.bubc/.bytecode).";
            LogError("%s", msg.c_str());
            return 1;
        }
#endif
    }

    if (mode == LaunchMode::RunBytecode)
    {
        if (!scriptFile)
        {
#ifdef BU_RUNNER_ONLY
            std::string msg = "Usage: runner <file.buc> [--dump] [--dump-file file] or runner --run-bc <file.buc> [--dump] [--dump-file file]";
            LogError("%s", msg.c_str());
            return 1;
#else
            std::string msg = "No bytecode file specified.";
            LogError("%s", msg.c_str());
          
            return 1;
#endif
        }
    }
#ifndef BU_RUNNER_ONLY
    else if (scriptFile != nullptr)
    {
        code = loadFile(scriptFile, false);
        if (code.empty())
        {
            std::string msg = "Could not load script: ";
            msg += (scriptFile ? scriptFile : "<null>");
            LogError("%s", msg.c_str());
          
            return 1;
        }
    }
#endif

#ifndef BU_RUNNER_ONLY
    if (mode == LaunchMode::RunSource && code.empty())
    {
        std::vector<std::string> defaultCandidates;
        defaultCandidates.reserve(8);
#ifdef __EMSCRIPTEN__
        defaultCandidates.emplace_back("/scripts/main.bu");
        defaultCandidates.emplace_back("/main.bu");
#endif
        defaultCandidates.emplace_back((exeDir / "scripts" / "main.bu").lexically_normal().string());
        defaultCandidates.emplace_back((exeDir.parent_path() / "scripts" / "main.bu").lexically_normal().string());
        defaultCandidates.emplace_back("scripts/main.bu");
        defaultCandidates.emplace_back("./scripts/main.bu");
        defaultCandidates.emplace_back("main.bu");
        defaultCandidates.emplace_back("../scripts/main.bu");

        for (const std::string &candidate : defaultCandidates)
        {
            code = loadFile(candidate.c_str(), true);
            if (!code.empty())
            {
                resolvedScriptFile = candidate;
                scriptFile = resolvedScriptFile.c_str();
                break;
            }
        }

        if (code.empty())
        {
            std::string msg = "No script file specified and no default found.";
            LogError("%s", msg.c_str());
          
            return 1;
        }
    }
#endif
    if (mode == LaunchMode::RunBytecode)
    {
        LogInfo("Using bytecode: %s", scriptFile ? scriptFile : "<none>");
    }
    else if (mode == LaunchMode::CompileBytecode)
    {
        LogInfo("Compiling script to bytecode: %s -> %s",
                 scriptFile ? scriptFile : "<none>",
                 bytecodeOutFile ? bytecodeOutFile : "<none>");
    }
    else
    {
        LogInfo("Using script: %s", scriptFile ? scriptFile : "<none>");
    }

    // Build include search paths anchored to the loaded main script location.
    char scriptPathAbs[512] = {0};
    if (scriptFile && isAbsolutePath(scriptFile))
    {
        snprintf(scriptPathAbs, sizeof(scriptPathAbs), "%s", scriptFile);
    }
    else if (scriptFile)
    {
        snprintf(scriptPathAbs, sizeof(scriptPathAbs), "%s/%s", Utils::GetWorkingDirectory(), scriptFile);
    }
    else
    {
        snprintf(scriptPathAbs, sizeof(scriptPathAbs), "%s", Utils::GetWorkingDirectory());
    }

    char scriptDir[512] = {0};
    char scriptParentDir[512] = {0};
    pathDirname(scriptPathAbs, scriptDir, sizeof(scriptDir));
    pathDirname(scriptDir, scriptParentDir, sizeof(scriptParentDir));

    ctx.pathCount = 0;
    auto addSearchPath = [&](const char *p)
    {
        if (!p || !*p || ctx.pathCount >= 8)
            return;
        for (int i = 0; i < ctx.pathCount; ++i)
        {
            if (strcmp(ctx.searchPaths[i], p) == 0)
                return;
        }
        ctx.searchPaths[ctx.pathCount++] = p;
    };

    addSearchPath(scriptDir);
    addSearchPath(scriptParentDir);
    addSearchPath("/scripts");
    addSearchPath("scripts");
    addSearchPath("./scripts");
    addSearchPath("../scripts");
    addSearchPath(".");

    vm.setFileLoader(multiPathFileLoader, &ctx);

    if (mode == LaunchMode::CompileBytecode)
    {
        bool compileOk = vm.compileToBytecode(code.c_str(), bytecodeOutFile, false);
        if (!compileOk)
        {
            std::string msg = "Failed to compile bytecode: ";
            msg += (bytecodeOutFile ? bytecodeOutFile : "<null>");
            LogError("%s", msg.c_str());
          
            return 1;
        }

        if (dumpToFile)
            vm.dumpToFile(dumpOutputFile);

        LogInfo( "Bytecode saved: %s", bytecodeOutFile ? bytecodeOutFile : "<none>");
        return 0;
    }

    

    bool scriptOk = false;
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
    try
    {
        if (mode == LaunchMode::RunBytecode)
            scriptOk = vm.loadBytecode(scriptFile);
        else
            scriptOk = vm.run(code.c_str(), false);
    }
    catch (const std::exception &e)
    {
        std::string msg = "Script exception while loading: ";
        msg += e.what();
        LogError("%s", msg.c_str());
      
 
        return 1;
    }
    catch (...)
    {
        std::string msg = "Unknown C++ exception while loading script.";
        LogError("%s", msg.c_str());
      
 
        return 1;
    }
#else
    if (mode == LaunchMode::RunBytecode)
        scriptOk = vm.loadBytecode(scriptFile);
    else
        scriptOk = vm.run(code.c_str(), false);
#endif

    if (!scriptOk)
    {
        if (mode == LaunchMode::RunBytecode)
            Error("Failed to execute bytecode: %s", scriptFile);
        else
            Error("Failed to execute script: %s", scriptFile);

        std::string msg = (mode == LaunchMode::RunBytecode)
                              ? "Failed to execute bytecode: "
                              : "Failed to execute script: ";
        msg += (scriptFile ? scriptFile : "<null>");
      
        LogError("%s", msg.c_str());
        return 1;
    }

    if (dumpToFile)
        vm.dumpToFile(dumpOutputFile);

 
    return 0;
}
