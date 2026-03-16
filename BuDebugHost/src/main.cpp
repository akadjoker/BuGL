#include "interpreter.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

struct FileLoaderContext
{
    std::vector<std::string> searchPaths;
    std::vector<unsigned char> bytes;
};

enum class LaunchMode
{
    RunSource,
    RunBytecode,
    CompileBytecode,
};

static bool hasSuffix(const std::string &value, const char *suffix)
{
    const size_t suffixLen = std::strlen(suffix);
    if (value.size() < suffixLen)
        return false;
    return value.compare(value.size() - suffixLen, suffixLen, suffix) == 0;
}

static bool isBytecodePath(const std::string &path)
{
    return hasSuffix(path, ".buc") || hasSuffix(path, ".bubc") || hasSuffix(path, ".bytecode");
}

static std::string getExecutablePath(const char *argv0)
{
#if defined(_WIN32)
    char buffer[1024] = {0};
    const DWORD len = GetModuleFileNameA(nullptr, buffer, static_cast<DWORD>(sizeof(buffer)));
    if (len > 0 && len < sizeof(buffer))
        return fs::path(buffer).lexically_normal().string();
#elif defined(__linux__)
    char buffer[1024] = {0};
    const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len > 0)
    {
        buffer[len] = '\0';
        return fs::path(buffer).lexically_normal().string();
    }
#elif defined(__APPLE__)
    char buffer[1024] = {0};
    uint32_t size = static_cast<uint32_t>(sizeof(buffer));
    if (_NSGetExecutablePath(buffer, &size) == 0)
        return fs::path(buffer).lexically_normal().string();
#endif

    if (argv0 && *argv0)
    {
        std::error_code ec;
        const fs::path absolute = fs::absolute(argv0, ec);
        if (!ec)
            return absolute.lexically_normal().string();
    }

    return fs::current_path().lexically_normal().string();
}

static fs::path findProjectRoot(const std::vector<fs::path> &starts)
{
    for (fs::path start : starts)
    {
        std::error_code ec;
        if (start.empty())
            continue;
        start = fs::weakly_canonical(start, ec);
        if (ec)
            start = start.lexically_normal();

        for (fs::path current = start; !current.empty(); current = current.parent_path())
        {
            if (fs::exists(current / "CMakeLists.txt") && fs::exists(current / "libbu") && fs::exists(current / "scripts"))
                return current;
            if (current == current.parent_path())
                break;
        }
    }

    return fs::current_path().lexically_normal();
}

static void addUniquePath(std::vector<std::string> &paths, const fs::path &path)
{
    if (path.empty())
        return;

    std::error_code ec;
    const std::string normalized = fs::weakly_canonical(path, ec).lexically_normal().string();
    const std::string fallback = path.lexically_normal().string();
    const std::string &candidate = ec ? fallback : normalized;
    if (candidate.empty())
        return;

    if (std::find(paths.begin(), paths.end(), candidate) == paths.end())
        paths.push_back(candidate);
}

static void addUniquePluginSearchPath(Interpreter &vm, std::vector<std::string> &paths, const fs::path &path)
{
    const size_t before = paths.size();
    addUniquePath(paths, path);
    if (paths.size() != before)
        vm.addPluginSearchPath(paths.back().c_str());
}

static std::string readTextFile(const fs::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return {};

    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

static bool readBinaryFile(const fs::path &path, std::vector<unsigned char> &bytes)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

static fs::path resolveExistingPath(const std::string &input, const std::vector<fs::path> &bases)
{
    std::error_code ec;
    const fs::path raw(input);
    if (raw.is_absolute() && fs::exists(raw))
        return fs::weakly_canonical(raw, ec);

    if (fs::exists(raw))
        return fs::weakly_canonical(raw, ec);

    for (const fs::path &base : bases)
    {
        const fs::path candidate = base / raw;
        if (fs::exists(candidate))
            return fs::weakly_canonical(candidate, ec);
    }

    return {};
}

static const char *multiPathFileLoader(const char *filename, size_t *outSize, void *userdata)
{
    if (!filename || !outSize || !userdata)
        return nullptr;

    FileLoaderContext *ctx = static_cast<FileLoaderContext *>(userdata);
    *outSize = 0;

    fs::path requested(filename);
    std::vector<fs::path> candidates;
    if (requested.is_absolute())
    {
        candidates.push_back(requested);
    }
    else
    {
        candidates.push_back(requested);
        std::string relativeName = filename;
        while (!relativeName.empty() && (relativeName[0] == '/' || relativeName[0] == '\\'))
            relativeName.erase(relativeName.begin());
        if (relativeName != filename)
            candidates.emplace_back(relativeName);

        for (const std::string &base : ctx->searchPaths)
        {
            candidates.emplace_back(fs::path(base) / requested);
            if (relativeName != filename)
                candidates.emplace_back(fs::path(base) / relativeName);
        }
    }

    for (const fs::path &candidate : candidates)
    {
        if (!fs::exists(candidate))
            continue;
        if (!readBinaryFile(candidate, ctx->bytes))
            continue;

        const size_t byteCount = ctx->bytes.size();
        ctx->bytes.push_back('\0');
        *outSize = byteCount;
        return reinterpret_cast<const char *>(ctx->bytes.data());
    }

    return nullptr;
}

static void printUsage(const char *argv0)
{
    const char *exe = (argv0 && *argv0) ? argv0 : "budebughost";
    std::fprintf(stderr,
                 "Usage:\n"
                 "  %s <script.bu> [--dump] [--debug-runtime]\n"
                 "  %s --run-bc <file.buc> [--debug-runtime]\n"
                 "  %s --compile-bc <input.bu> <output.buc> [--dump]\n",
                 exe,
                 exe,
                 exe);
}

int main(int argc, char *argv[])
{
    const fs::path exePath = fs::path(getExecutablePath(argc > 0 ? argv[0] : nullptr));
    const fs::path exeDir = exePath.has_parent_path() ? exePath.parent_path() : fs::current_path();
    const fs::path cwd = fs::current_path();
    const fs::path projectRoot = findProjectRoot({cwd, exeDir, exeDir.parent_path()});

    LaunchMode mode = LaunchMode::RunSource;
    std::string scriptArg;
    std::string bytecodeOutputArg;
    bool dump = false;
    bool debugRuntime = false;

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (!arg)
            continue;

        if (std::strcmp(arg, "--dump") == 0 || std::strcmp(arg, "--dump-bytecode") == 0)
        {
            dump = true;
            continue;
        }

        if (std::strcmp(arg, "--debug-runtime") == 0)
        {
            debugRuntime = true;
            continue;
        }

        if (std::strcmp(arg, "--compile-bc") == 0 || std::strcmp(arg, "--compile-bytecode") == 0)
        {
            if (i + 2 >= argc)
            {
                printUsage(argc > 0 ? argv[0] : nullptr);
                return 1;
            }
            mode = LaunchMode::CompileBytecode;
            scriptArg = argv[++i];
            bytecodeOutputArg = argv[++i];
            continue;
        }

        if (std::strcmp(arg, "--run-bc") == 0 || std::strcmp(arg, "--run-bytecode") == 0)
        {
            if (i + 1 >= argc)
            {
                printUsage(argc > 0 ? argv[0] : nullptr);
                return 1;
            }
            mode = LaunchMode::RunBytecode;
            scriptArg = argv[++i];
            continue;
        }

        if (arg[0] == '-')
        {
            std::fprintf(stderr, "Unknown option: %s\n", arg);
            printUsage(argc > 0 ? argv[0] : nullptr);
            return 1;
        }

        if (!scriptArg.empty())
        {
            std::fprintf(stderr, "Unexpected extra argument: %s\n", arg);
            printUsage(argc > 0 ? argv[0] : nullptr);
            return 1;
        }

        scriptArg = arg;
        if (isBytecodePath(scriptArg))
            mode = LaunchMode::RunBytecode;
    }

    if (scriptArg.empty())
    {
        if (mode == LaunchMode::RunSource)
        {
            const std::vector<fs::path> defaults = {
                projectRoot / "scripts" / "main.bu",
                cwd / "scripts" / "main.bu",
                exeDir / "scripts" / "main.bu",
                exeDir.parent_path() / "scripts" / "main.bu",
            };
            for (const fs::path &candidate : defaults)
            {
                if (fs::exists(candidate))
                {
                    scriptArg = candidate.lexically_normal().string();
                    break;
                }
            }
        }

        if (scriptArg.empty())
        {
            printUsage(argc > 0 ? argv[0] : nullptr);
            return 1;
        }
    }

    const std::vector<fs::path> searchBases = {
        cwd,
        projectRoot,
        projectRoot / "scripts",
        exeDir,
        exeDir.parent_path(),
        exeDir / "scripts",
        exeDir.parent_path() / "scripts",
    };

    const fs::path scriptPath = resolveExistingPath(scriptArg, searchBases);
    if (scriptPath.empty())
    {
        std::fprintf(stderr, "Could not resolve input path: %s\n", scriptArg.c_str());
        return 1;
    }

    Interpreter vm;
    vm.registerAll();
    vm.setDebugMode(debugRuntime);

    std::vector<std::string> pluginSearchPaths;
    addUniquePluginSearchPath(vm, pluginSearchPaths, exeDir / "plugins");
    addUniquePluginSearchPath(vm, pluginSearchPaths, exeDir / "modules");
    addUniquePluginSearchPath(vm, pluginSearchPaths, projectRoot / "bin" / "plugins");
    addUniquePluginSearchPath(vm, pluginSearchPaths, projectRoot / "bin" / "modules");
    addUniquePluginSearchPath(vm, pluginSearchPaths, projectRoot / "plugins");
    addUniquePluginSearchPath(vm, pluginSearchPaths, projectRoot / "modules");
    addUniquePluginSearchPath(vm, pluginSearchPaths, cwd / "bin" / "plugins");
    addUniquePluginSearchPath(vm, pluginSearchPaths, cwd / "bin" / "modules");

    FileLoaderContext loader;
    addUniquePath(loader.searchPaths, scriptPath.parent_path());
    addUniquePath(loader.searchPaths, scriptPath.parent_path().parent_path());
    addUniquePath(loader.searchPaths, projectRoot / "scripts");
    addUniquePath(loader.searchPaths, cwd / "scripts");
    addUniquePath(loader.searchPaths, projectRoot);
    addUniquePath(loader.searchPaths, cwd);
    vm.setFileLoader(multiPathFileLoader, &loader);

    if (mode == LaunchMode::RunBytecode)
    {
        std::printf("BuDebugHost: running bytecode %s\n", scriptPath.string().c_str());
        return vm.loadBytecode(scriptPath.string().c_str()) ? 0 : 1;
    }

    const std::string source = readTextFile(scriptPath);
    if (source.empty())
    {
        std::fprintf(stderr, "Could not load source file: %s\n", scriptPath.string().c_str());
        return 1;
    }

    if (mode == LaunchMode::CompileBytecode)
    {
        fs::path outputPath(bytecodeOutputArg);
        if (!outputPath.is_absolute())
            outputPath = projectRoot / outputPath;
        if (outputPath.has_parent_path())
        {
            std::error_code ec;
            fs::create_directories(outputPath.parent_path(), ec);
        }

        std::printf("BuDebugHost: compiling %s -> %s\n",
                    scriptPath.string().c_str(),
                    outputPath.lexically_normal().string().c_str());
        return vm.compileToBytecode(source.c_str(), outputPath.lexically_normal().string().c_str(), dump) ? 0 : 1;
    }

    std::printf("BuDebugHost: running source %s\n", scriptPath.string().c_str());
    return vm.run(source.c_str(), dump) ? 0 : 1;
}
