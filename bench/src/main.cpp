#include "interpreter.hpp"
#include "opcode.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct FileLoaderContext
{
    std::vector<std::string> searchPaths;
    std::vector<unsigned char> bytes;
};

struct BenchmarkCase
{
    fs::path path;
    std::string name;
};

enum class BenchmarkMode
{
    Cold,
    Hot,
    Both,
};

struct BenchmarkOptions
{
    int warmupIterations{1};
    int measureIterations{5};
    BenchmarkMode mode{BenchmarkMode::Both};
    bool listOnly{false};
    std::string filter;
    std::vector<fs::path> explicitPaths;
};

struct BenchmarkStats
{
    int iterations{0};
    double minMs{0.0};
    double maxMs{0.0};
    double avgMs{0.0};
    double totalMs{0.0};
    double nsPerOp{0.0};
    uint64_t benchOps{0};
    std::string lastValue{"nil"};
};

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

static fs::path normalizePath(const fs::path &path)
{
    if (path.empty())
        return {};

    std::error_code ec;
    fs::path normalized = fs::weakly_canonical(path, ec);
    if (ec)
        return path.lexically_normal();
    return normalized.lexically_normal();
}

static fs::path executablePath(const char *argv0)
{
    if (!argv0 || !*argv0)
        return {};

    std::error_code ec;
    fs::path path = fs::absolute(fs::path(argv0), ec);
    if (ec)
        path = fs::path(argv0);
    return normalizePath(path);
}

static fs::path resolveProjectRoot(const char *argv0)
{
    std::vector<fs::path> roots;
    roots.push_back(fs::current_path());

    const fs::path exePath = executablePath(argv0);
    if (!exePath.empty())
    {
        const fs::path exeDir = exePath.parent_path();
        roots.push_back(exeDir);
        if (!exeDir.empty())
            roots.push_back(exeDir.parent_path());
    }

#ifdef BULANG_BENCH_SOURCE_DIR
    roots.push_back(fs::path(BULANG_BENCH_SOURCE_DIR));
#endif

    for (const fs::path &root : roots)
    {
        if (root.empty())
            continue;

        const fs::path normalized = normalizePath(root);
        if (fs::exists(normalized / "bench" / "scripts"))
            return normalized;
    }

    return normalizePath(fs::current_path());
}

static std::vector<BenchmarkCase> discoverBenchmarks(const fs::path &benchRoot)
{
    std::vector<BenchmarkCase> benches;
    if (!fs::exists(benchRoot))
        return benches;

    for (const fs::directory_entry &entry : fs::directory_iterator(benchRoot))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path path = entry.path();
        const std::string filename = path.filename().string();
        if (path.extension() != ".bu")
            continue;
        if (filename.rfind("bench_", 0) != 0)
            continue;

        benches.push_back({normalizePath(path), path.stem().string()});
    }

    std::sort(benches.begin(), benches.end(), [](const BenchmarkCase &a, const BenchmarkCase &b)
              { return a.name < b.name; });
    return benches;
}

static fs::path resolveInputPath(const fs::path &path, const fs::path &projectRoot)
{
    if (path.is_absolute())
        return normalizePath(path);

    const fs::path cwdCandidate = normalizePath(fs::current_path() / path);
    if (fs::exists(cwdCandidate))
        return cwdCandidate;

    return normalizePath(projectRoot / path);
}

static bool matchesFilter(const BenchmarkCase &benchCase, const BenchmarkOptions &options)
{
    if (!options.filter.empty() && benchCase.name.find(options.filter) == std::string::npos)
        return false;
    return true;
}

static void printUsage(const char *argv0)
{
    const char *exe = (argv0 && *argv0) ? argv0 : "bulang-bench";
    std::fprintf(stderr,
                 "Usage: %s [options] [bench/scripts/bench_name.bu ...]\n"
                 "Options:\n"
                 "  --list               list available benchmarks\n"
                 "  --filter <text>      run only benchmarks whose name contains text\n"
                 "  --mode <cold|hot|both>\n"
                 "  --warmup <n>         warmup iterations per benchmark (default: 1)\n"
                 "  --iterations <n>     measured iterations per benchmark (default: 5)\n",
                 exe);
}

static bool parsePositiveInt(const char *text, int *out)
{
    if (!text || !out)
        return false;

    char *end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || (end && *end != '\0') || value <= 0)
        return false;

    *out = static_cast<int>(value);
    return true;
}

static bool parseOptions(int argc, char *argv[], BenchmarkOptions *out)
{
    if (!out)
        return false;

    BenchmarkOptions options;
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (!arg)
            continue;

        if (std::strcmp(arg, "--list") == 0)
        {
            options.listOnly = true;
            continue;
        }

        if (std::strcmp(arg, "--filter") == 0)
        {
            if (i + 1 >= argc)
                return false;
            options.filter = argv[++i];
            continue;
        }

        if (std::strcmp(arg, "--warmup") == 0)
        {
            if (i + 1 >= argc || !parsePositiveInt(argv[++i], &options.warmupIterations))
                return false;
            continue;
        }

        if (std::strcmp(arg, "--iterations") == 0)
        {
            if (i + 1 >= argc || !parsePositiveInt(argv[++i], &options.measureIterations))
                return false;
            continue;
        }

        if (std::strcmp(arg, "--mode") == 0)
        {
            if (i + 1 >= argc)
                return false;

            const std::string mode = argv[++i];
            if (mode == "cold")
                options.mode = BenchmarkMode::Cold;
            else if (mode == "hot")
                options.mode = BenchmarkMode::Hot;
            else if (mode == "both")
                options.mode = BenchmarkMode::Both;
            else
                return false;
            continue;
        }

        if (arg[0] == '-')
            return false;

        options.explicitPaths.push_back(fs::path(arg));
    }

    *out = options;
    return true;
}

static void configureVm(Interpreter &vm,
                        FileLoaderContext &loader,
                        const fs::path &projectRoot,
                        const fs::path &scriptPath)
{
    addUniquePath(loader.searchPaths, scriptPath.parent_path());
    addUniquePath(loader.searchPaths, projectRoot / "bench" / "scripts");
    addUniquePath(loader.searchPaths, projectRoot / "tests" / "scripts");
    addUniquePath(loader.searchPaths, projectRoot / "tests" / "scripts" / "helpers");
    addUniquePath(loader.searchPaths, projectRoot);

    vm.registerAll();
    vm.setShutdownSummaryEnabled(false);
    vm.setFileLoader(multiPathFileLoader, &loader);
    vm.addPluginSearchPath((projectRoot / "bin").string().c_str());
    vm.addPluginSearchPath((projectRoot / "bin" / "plugins").string().c_str());
}

static bool initializeVmForBenchmark(Interpreter &vm,
                                     FileLoaderContext &loader,
                                     const fs::path &projectRoot,
                                     const BenchmarkCase &benchCase,
                                     const std::string &source)
{
    configureVm(vm, loader, projectRoot, benchCase.path);
    if (!vm.run(source.c_str(), false))
        return false;
    vm.setTop(0);
    return true;
}

static uint64_t readUnsignedGlobal(Interpreter &vm, const char *name)
{
    Value value;
    if (!vm.tryGetGlobal(name, &value))
        return 0;
    if (value.isInt())
        return static_cast<uint64_t>(value.asInt());
    if (value.isUInt())
        return static_cast<uint64_t>(value.asUInt());
    if (value.isDouble())
        return static_cast<uint64_t>(value.asDouble());
    return 0;
}

static bool invokeBench(Interpreter &vm, Value *outResult, uint64_t *outBenchOps)
{
    if (!outResult || !outBenchOps)
        return false;

    *outResult = Value{};
    *outBenchOps = 0;
    vm.setTop(0);
    const int startTop = vm.getTop();
    if (!vm.callFunctionAuto("bench", 0))
        return false;

    if (vm.getTop() > startTop)
        *outResult = vm.pop();
    else
        vm.tryGetGlobal("__bench_result", outResult);
    if (vm.getTop() != startTop)
        vm.setTop(startTop);
    *outBenchOps = readUnsignedGlobal(vm, "__bench_ops");
    return true;
}

static std::string valueToString(const Value &value)
{
    char buffer[256] = {0};
    valueToBuffer(value, buffer, sizeof(buffer));
    return buffer[0] ? std::string(buffer) : std::string("nil");
}

template <typename Fn>
static bool runIterations(int warmupCount, int measureCount, Fn &&fn, BenchmarkStats *outStats)
{
    if (!outStats)
        return false;

    *outStats = {};
    BenchmarkStats &stats = *outStats;

    for (int i = 0; i < warmupCount; ++i)
    {
        if (!fn(false, nullptr))
            return false;
    }

    stats.iterations = measureCount;

    for (int i = 0; i < measureCount; ++i)
    {
        double elapsedMs = 0.0;
        if (!fn(true, &elapsedMs))
            return false;

        if (i == 0 || elapsedMs < stats.minMs)
            stats.minMs = elapsedMs;
        if (i == 0 || elapsedMs > stats.maxMs)
            stats.maxMs = elapsedMs;
        stats.totalMs += elapsedMs;
    }

    stats.avgMs = measureCount > 0 ? (stats.totalMs / static_cast<double>(measureCount)) : 0.0;
    if (stats.benchOps > 0)
        stats.nsPerOp = (stats.totalMs * 1000000.0) / static_cast<double>(stats.benchOps);
    return true;
}

static bool runColdBenchmark(const fs::path &projectRoot,
                             const BenchmarkCase &benchCase,
                             const std::string &source,
                             const BenchmarkOptions &options,
                             BenchmarkStats *outStats)
{
    BenchmarkStats stats;
    auto runner = [&](bool measure, double *elapsedMs) -> bool
    {
        Interpreter vm;
        FileLoaderContext loader;
        if (!initializeVmForBenchmark(vm, loader, projectRoot, benchCase, source))
            return false;

        const auto begin = std::chrono::steady_clock::now();
        Value result;
        uint64_t benchOps = 0;
        if (!invokeBench(vm, &result, &benchOps))
            return false;
        const auto end = std::chrono::steady_clock::now();

        if (measure && elapsedMs)
        {
            *elapsedMs = std::chrono::duration<double, std::milli>(end - begin).count();
            stats.lastValue = valueToString(result);
            stats.benchOps += benchOps;
        }

        return true;
    };

    if (!runIterations(options.warmupIterations, options.measureIterations, runner, &stats))
        return false;

    *outStats = stats;
    return true;
}

static bool runHotBenchmark(const fs::path &projectRoot,
                            const BenchmarkCase &benchCase,
                            const std::string &source,
                            const BenchmarkOptions &options,
                            BenchmarkStats *outStats)
{
    Interpreter vm;
    FileLoaderContext loader;
    if (!initializeVmForBenchmark(vm, loader, projectRoot, benchCase, source))
        return false;

    BenchmarkStats stats;
    auto runner = [&](bool measure, double *elapsedMs) -> bool
    {
        const auto begin = std::chrono::steady_clock::now();
        Value result;
        uint64_t benchOps = 0;
        if (!invokeBench(vm, &result, &benchOps))
            return false;
        const auto end = std::chrono::steady_clock::now();

        if (measure && elapsedMs)
        {
            *elapsedMs = std::chrono::duration<double, std::milli>(end - begin).count();
            stats.lastValue = valueToString(result);
            stats.benchOps += benchOps;
        }

        return true;
    };

    if (!runIterations(options.warmupIterations, options.measureIterations, runner, &stats))
        return false;

    *outStats = stats;
    return true;
}

static void printStatsRow(const std::string &modeLabel, const BenchmarkCase &benchCase, const BenchmarkStats &stats)
{
    std::cout << std::left << std::setw(10) << modeLabel
              << std::setw(28) << benchCase.name
              << std::right << std::setw(8) << stats.iterations
              << std::setw(12) << stats.avgMs
              << std::setw(14) << stats.benchOps
              << std::setw(12) << std::fixed << std::setprecision(2) << stats.nsPerOp
              << "   " << stats.lastValue
              << "\n";
}

static std::vector<BenchmarkCase> collectSelectedBenchmarks(const fs::path &projectRoot,
                                                            const BenchmarkOptions &options)
{
    std::vector<BenchmarkCase> benches;
    if (options.explicitPaths.empty())
    {
        benches = discoverBenchmarks(projectRoot / "bench" / "scripts");
    }
    else
    {
        for (const fs::path &input : options.explicitPaths)
        {
            const fs::path resolved = resolveInputPath(input, projectRoot);
            if (!fs::exists(resolved))
                continue;
            benches.push_back({resolved, resolved.stem().string()});
        }
    }

    std::vector<BenchmarkCase> filtered;
    for (const BenchmarkCase &benchCase : benches)
    {
        if (matchesFilter(benchCase, options))
            filtered.push_back(benchCase);
    }

    std::sort(filtered.begin(), filtered.end(), [](const BenchmarkCase &a, const BenchmarkCase &b)
              { return a.name < b.name; });
    return filtered;
}

int main(int argc, char *argv[])
{
    BenchmarkOptions options;
    if (!parseOptions(argc, argv, &options))
    {
        printUsage(argc > 0 ? argv[0] : nullptr);
        return 1;
    }

    const fs::path projectRoot = resolveProjectRoot(argc > 0 ? argv[0] : nullptr);
    const std::vector<BenchmarkCase> benchmarks = collectSelectedBenchmarks(projectRoot, options);
    if (benchmarks.empty())
    {
        std::fprintf(stderr, "No benchmarks matched.\n");
        return 1;
    }

    if (options.listOnly)
    {
        for (const BenchmarkCase &benchCase : benchmarks)
            std::cout << benchCase.name << "  " << benchCase.path.string() << "\n";
        return 0;
    }

    std::cout << "BuLang VM benchmarks\n";
    std::cout << "Project root: " << projectRoot.string() << "\n";
    std::cout << "Warmup: " << options.warmupIterations
              << "  Iterations: " << options.measureIterations << "\n\n";
    std::cout << std::left << std::setw(10) << "mode"
              << std::setw(28) << "benchmark"
              << std::right << std::setw(8) << "iters"
              << std::setw(12) << "avg ms"
              << std::setw(14) << "ops"
              << std::setw(12) << "ns/op"
              << "   result\n";

    const int previousLogSeverity = GetLogMinimumSeverity();
    SetLogMinimumSeverity(1);

    int failures = 0;
    for (const BenchmarkCase &benchCase : benchmarks)
    {
        const std::string source = readTextFile(benchCase.path);
        if (source.empty())
        {
            std::fprintf(stderr, "[FAIL] %s: could not load benchmark script\n", benchCase.name.c_str());
            failures++;
            continue;
        }

        if (options.mode == BenchmarkMode::Cold || options.mode == BenchmarkMode::Both)
        {
            BenchmarkStats stats;
            if (!runColdBenchmark(projectRoot, benchCase, source, options, &stats))
            {
                std::fprintf(stderr, "[FAIL] cold %s\n", benchCase.name.c_str());
                failures++;
            }
            else
            {
                printStatsRow("cold", benchCase, stats);
            }
        }

        if (options.mode == BenchmarkMode::Hot || options.mode == BenchmarkMode::Both)
        {
            BenchmarkStats stats;
            if (!runHotBenchmark(projectRoot, benchCase, source, options, &stats))
            {
                std::fprintf(stderr, "[FAIL] hot %s\n", benchCase.name.c_str());
                failures++;
            }
            else
            {
                printStatsRow("hot", benchCase, stats);
            }
        }
    }

    SetLogMinimumSeverity(previousLogSeverity);

    return failures == 0 ? 0 : 1;
}
