#include "interpreter.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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

static const char *pauseReasonName(DebugPauseReason reason)
{
    switch (reason)
    {
    case DebugPauseReason::BREAKPOINT:
        return "breakpoint";
    case DebugPauseReason::STEP:
        return "step";
    case DebugPauseReason::MANUAL:
        return "manual";
    case DebugPauseReason::NONE:
    default:
        return "pause";
    }
}

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
                 "  %s <script.bu> [--dump] [--debug-runtime] [--debugger] [--step] [--break <line>]\n"
                 "  %s --run-bc <file.buc> [--debug-runtime]\n"
                 "  %s --compile-bc <input.bu> <output.buc> [--dump]\n",
                 exe,
                 exe,
                 exe);
}

static void printDebuggerHelp()
{
    std::printf("Debugger commands:\n");
    std::printf("  c, continue   continue execution\n");
    std::printf("  s, step       step to next pause point\n");
    std::printf("  bt, frames    show call frames\n");
    std::printf("  locals [n]    show frame locals for depth n (default 0)\n");
    std::printf("  stack         show VM stack\n");
    std::printf("  globals       show globals\n");
    std::printf("  p, print x    print global 'x' or local '$0'\n");
    std::printf("  w, watch x    watch global 'x' or local '$0'\n");
    std::printf("  uw, unwatch x remove watch for 'x' or '$0'\n");
    std::printf("  watches       list active watches\n");
    std::printf("  b <line>      add breakpoint\n");
    std::printf("  rb <line>     remove breakpoint\n");
    std::printf("  q, quit       stop debugging\n");
    std::printf("  h, help       show this help\n");
}

enum class DebuggerPromptAction
{
    Continue,
    Quit,
};

enum class DebugValueKind
{
    Global,
    LocalSlot,
};

struct DebugValueRef
{
    DebugValueKind kind{DebugValueKind::Global};
    std::string expr;
    int slotIndex{-1};
};

struct DebugWatch
{
    DebugValueRef ref;
    bool hasLastValue{false};
    bool wasResolved{false};
    Value lastValue{};
};

static bool parseDebugValueRef(const std::string &expr, DebugValueRef *out, std::string *error)
{
    if (!out)
        return false;

    if (expr.empty())
    {
        if (error)
            *error = "Missing variable expression";
        return false;
    }

    out->expr = expr;
    out->slotIndex = -1;
    out->kind = DebugValueKind::Global;

    if (expr[0] != '$')
        return true;

    char *end = nullptr;
    const long slot = std::strtol(expr.c_str() + 1, &end, 10);
    if (end == expr.c_str() + 1 || (end && *end != '\0') || slot < 0)
    {
        if (error)
            *error = "Local slots use '$<index>', for example '$0'";
        return false;
    }

    out->kind = DebugValueKind::LocalSlot;
    out->slotIndex = static_cast<int>(slot);
    return true;
}

static bool resolveDebugValueRef(Interpreter &vm, const DebugValueRef &ref, Value *out, std::string *error)
{
    if (!out)
        return false;

    *out = Value{};
    switch (ref.kind)
    {
    case DebugValueKind::LocalSlot:
        if (!vm.getFrameSlotValue(0, ref.slotIndex, out))
        {
            if (error)
                *error = "Local slot not available in the current frame";
            return false;
        }
        return true;
    case DebugValueKind::Global:
    default:
        if (!vm.tryGetGlobal(ref.expr.c_str(), out))
        {
            if (error)
                *error = "Global not found";
            return false;
        }
        return true;
    }
}

static void printResolvedValue(const std::string &label, const Value &value)
{
    std::printf("  %s = ", label.c_str());
    printValue(value);
    std::printf("\n");
}

static void reportDebuggerWatchChanges(Interpreter &vm, std::vector<DebugWatch> &watches)
{
    for (DebugWatch &watch : watches)
    {
        std::string error;
        Value value;
        if (!resolveDebugValueRef(vm, watch.ref, &value, &error))
        {
            if (watch.wasResolved)
                std::printf("[watch] %s unavailable\n", watch.ref.expr.c_str());
            watch.wasResolved = false;
            watch.hasLastValue = false;
            continue;
        }

        if (!watch.wasResolved || !watch.hasLastValue)
        {
            std::printf("[watch]");
            printResolvedValue(watch.ref.expr, value);
            watch.lastValue = value;
            watch.hasLastValue = true;
            watch.wasResolved = true;
            continue;
        }

        if (!valuesEqual(watch.lastValue, value))
        {
            char previous[256] = {0};
            char current[256] = {0};
            valueToBuffer(watch.lastValue, previous, sizeof(previous));
            valueToBuffer(value, current, sizeof(current));
            std::printf("[watch] %s changed: %s -> %s\n",
                        watch.ref.expr.c_str(),
                        previous,
                        current);
            watch.lastValue = value;
        }
    }
}

static DebuggerPromptAction runDebuggerPrompt(Interpreter &vm, std::vector<DebugWatch> &watches)
{
    const DebugPauseState pause = vm.getDebugPauseState();
    std::printf("\nPaused on %s at %s:%d\n",
                pauseReasonName(pause.reason),
                pause.location.functionName ? pause.location.functionName : "<script>",
                pause.location.line);
    reportDebuggerWatchChanges(vm, watches);

    std::string line;
    while (true)
    {
        std::printf("(budebug) ");
        std::fflush(stdout);

        if (!std::getline(std::cin, line))
            return DebuggerPromptAction::Quit;

        std::istringstream input(line);
        std::string command;
        input >> command;
        if (command.empty())
            continue;

        if (command == "c" || command == "continue")
        {
            vm.continueDebug();
            return DebuggerPromptAction::Continue;
        }

        if (command == "s" || command == "step")
        {
            vm.stepDebug();
            return DebuggerPromptAction::Continue;
        }

        if (command == "bt" || command == "frames")
        {
            vm.dumpCallFrames();
            continue;
        }

        if (command == "locals")
        {
            int frameDepth = 0;
            if ((input >> frameDepth) && frameDepth < 0)
            {
                std::fprintf(stderr, "Expected non-negative frame depth after 'locals'\n");
                continue;
            }
            vm.dumpFrameLocals(frameDepth);
            continue;
        }

        if (command == "stack")
        {
            vm.printStack();
            continue;
        }

        if (command == "globals")
        {
            vm.dumpGlobals();
            continue;
        }

        if (command == "b")
        {
            int breakpoint = 0;
            if (!(input >> breakpoint) || breakpoint <= 0)
            {
                std::fprintf(stderr, "Expected positive line number after 'b'\n");
                continue;
            }
            vm.addDebugBreakpoint(breakpoint);
            std::printf("Breakpoint added at line %d\n", breakpoint);
            continue;
        }

        if (command == "rb")
        {
            int breakpoint = 0;
            if (!(input >> breakpoint) || breakpoint <= 0)
            {
                std::fprintf(stderr, "Expected positive line number after 'rb'\n");
                continue;
            }
            vm.removeDebugBreakpoint(breakpoint);
            std::printf("Breakpoint removed from line %d\n", breakpoint);
            continue;
        }

        if (command == "print" || command == "p")
        {
            std::string expr;
            if (!(input >> expr))
            {
                std::fprintf(stderr, "Expected global name or $slot after '%s'\n", command.c_str());
                continue;
            }

            DebugValueRef ref;
            std::string error;
            if (!parseDebugValueRef(expr, &ref, &error))
            {
                std::fprintf(stderr, "%s\n", error.c_str());
                continue;
            }

            Value value;
            if (!resolveDebugValueRef(vm, ref, &value, &error))
            {
                std::fprintf(stderr, "%s: %s\n", expr.c_str(), error.c_str());
                continue;
            }

            printResolvedValue(expr, value);
            continue;
        }

        if (command == "watch" || command == "w")
        {
            std::string expr;
            if (!(input >> expr))
            {
                std::fprintf(stderr, "Expected global name or $slot after '%s'\n", command.c_str());
                continue;
            }

            DebugValueRef ref;
            std::string error;
            if (!parseDebugValueRef(expr, &ref, &error))
            {
                std::fprintf(stderr, "%s\n", error.c_str());
                continue;
            }

            Value value;
            if (!resolveDebugValueRef(vm, ref, &value, &error))
            {
                std::fprintf(stderr, "%s: %s\n", expr.c_str(), error.c_str());
                continue;
            }

            const auto existing = std::find_if(watches.begin(),
                                               watches.end(),
                                               [&](const DebugWatch &watch)
                                               {
                                                   return watch.ref.kind == ref.kind &&
                                                          watch.ref.expr == ref.expr;
                                               });
            if (existing != watches.end())
            {
                std::printf("Watch already exists for %s\n", expr.c_str());
                continue;
            }

            DebugWatch watch;
            watch.ref = ref;
            watch.lastValue = value;
            watch.hasLastValue = true;
            watch.wasResolved = true;
            watches.push_back(watch);
            std::printf("Watching %s\n", expr.c_str());
            printResolvedValue(expr, value);
            continue;
        }

        if (command == "unwatch" || command == "uw")
        {
            std::string expr;
            if (!(input >> expr))
            {
                std::fprintf(stderr, "Expected watched expression after '%s'\n", command.c_str());
                continue;
            }

            const auto it = std::find_if(watches.begin(),
                                         watches.end(),
                                         [&](const DebugWatch &watch)
                                         {
                                             return watch.ref.expr == expr;
                                         });
            if (it == watches.end())
            {
                std::fprintf(stderr, "No watch found for %s\n", expr.c_str());
                continue;
            }

            watches.erase(it);
            std::printf("Stopped watching %s\n", expr.c_str());
            continue;
        }

        if (command == "watches")
        {
            if (watches.empty())
            {
                std::printf("(no watches)\n");
                continue;
            }

            for (DebugWatch &watch : watches)
            {
                std::string error;
                Value value;
                if (!resolveDebugValueRef(vm, watch.ref, &value, &error))
                {
                    std::printf("  %s = <unavailable>\n", watch.ref.expr.c_str());
                    continue;
                }
                printResolvedValue(watch.ref.expr, value);
            }
            continue;
        }

        if (command == "q" || command == "quit")
            return DebuggerPromptAction::Quit;

        if (command == "h" || command == "help")
        {
            printDebuggerHelp();
            continue;
        }

        std::fprintf(stderr, "Unknown command: %s\n", command.c_str());
    }
}

int main(int argc, char *argv[])
{
    const fs::path exePath = fs::path(getExecutablePath(argc > 0 ? argv[0] : nullptr));
    const fs::path exeDir = exePath.has_parent_path() ? exePath.parent_path() : fs::current_path();
    const fs::path cwd = fs::current_path();

    LaunchMode mode = LaunchMode::RunSource;
    std::string scriptArg;
    std::string bytecodeOutputArg;
    bool dump = false;
    bool debugRuntime = false;
    bool debuggerEnabled = false;
    bool startStepping = false;
    std::vector<int> debugBreakpoints;
    std::vector<DebugWatch> debugWatches;

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

        if (std::strcmp(arg, "--debugger") == 0 || std::strcmp(arg, "--debug") == 0)
        {
            debuggerEnabled = true;
            continue;
        }

        if (std::strcmp(arg, "--step") == 0)
        {
            debuggerEnabled = true;
            startStepping = true;
            continue;
        }

        if (std::strcmp(arg, "--break") == 0)
        {
            if (i + 1 >= argc)
            {
                printUsage(argc > 0 ? argv[0] : nullptr);
                return 1;
            }

            debuggerEnabled = true;
            const int breakpoint = std::atoi(argv[++i]);
            if (breakpoint <= 0)
            {
                std::fprintf(stderr, "Invalid breakpoint line: %s\n", argv[i]);
                return 1;
            }

            debugBreakpoints.push_back(breakpoint);
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
                cwd / "scripts" / "main.bu",
                cwd / "main.bu",
                exeDir / "scripts" / "main.bu",
                exeDir.parent_path() / "scripts" / "main.bu",
                exeDir / "main.bu",
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
        cwd / "scripts",
        exeDir,
        exeDir / "scripts",
        exeDir.parent_path(),
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

    FileLoaderContext loader;
    addUniquePath(loader.searchPaths, scriptPath.parent_path());
    addUniquePath(loader.searchPaths, scriptPath.parent_path().parent_path());
    addUniquePath(loader.searchPaths, cwd / "scripts");
    addUniquePath(loader.searchPaths, cwd);
    addUniquePath(loader.searchPaths, exeDir / "scripts");
    addUniquePath(loader.searchPaths, exeDir);
    addUniquePath(loader.searchPaths, exeDir.parent_path() / "scripts");
    addUniquePath(loader.searchPaths, exeDir.parent_path());
    vm.setFileLoader(multiPathFileLoader, &loader);
    vm.addPluginSearchPath((cwd / "bin").string().c_str());
    vm.addPluginSearchPath((cwd / "bin" / "plugins").string().c_str());
    vm.addPluginSearchPath(exeDir.string().c_str());
    vm.addPluginSearchPath((exeDir / "plugins").string().c_str());

    if (mode == LaunchMode::RunBytecode)
    {
        if (debuggerEnabled)
        {
            std::fprintf(stderr, "Debugger currently supports source scripts only.\n");
            return 1;
        }

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
            outputPath = (cwd / outputPath).lexically_normal();
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

    if (debuggerEnabled)
    {
        vm.enableDebugger(true);
        std::printf("BuDebugHost: debugging source %s\n", scriptPath.string().c_str());
        if (!vm.prepareSourceRun(source.c_str(), dump))
            return 1;

        for (size_t i = 0; i < debugBreakpoints.size(); ++i)
            vm.addDebugBreakpoint(debugBreakpoints[i]);
        if (startStepping)
            vm.stepDebug();

        while (!vm.isMainProcessFinished())
        {
            if (!vm.continueExecution())
                return 1;

            if (vm.isDebugPaused())
            {
                if (runDebuggerPrompt(vm, debugWatches) == DebuggerPromptAction::Quit)
                    return 0;
            }
        }

        return 0;
    }

    std::printf("BuDebugHost: running source %s\n", scriptPath.string().c_str());
    return vm.run(source.c_str(), dump) ? 0 : 1;
}
