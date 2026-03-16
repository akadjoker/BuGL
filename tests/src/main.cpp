#include "interpreter.hpp"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct FileLoaderContext
{
    std::vector<std::string> searchPaths;
    std::vector<unsigned char> bytes;
};

struct TestCase
{
    fs::path path;
    std::string name;
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

static std::vector<TestCase> discoverTests(const fs::path &testsRoot)
{
    std::vector<TestCase> tests;
    if (!fs::exists(testsRoot))
        return tests;

    for (const fs::directory_entry &entry : fs::directory_iterator(testsRoot))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path path = entry.path();
        const std::string filename = path.filename().string();
        if (path.extension() != ".bu")
            continue;
        if (filename.rfind("test_", 0) != 0)
            continue;

        tests.push_back({path, path.stem().string()});
    }

    std::sort(tests.begin(), tests.end(), [](const TestCase &a, const TestCase &b)
              { return a.name < b.name; });
    return tests;
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

#ifdef BULANG_TEST_SOURCE_DIR
    roots.push_back(fs::path(BULANG_TEST_SOURCE_DIR));
#endif

    for (const fs::path &root : roots)
    {
        if (root.empty())
            continue;

        const fs::path normalized = normalizePath(root);
        if (fs::exists(normalized / "tests" / "scripts"))
            return normalized;
    }

    return normalizePath(fs::current_path());
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

static bool runTest(const fs::path &projectRoot, const TestCase &testCase)
{
    const std::string source = readTextFile(testCase.path);
    if (source.empty())
    {
        std::fprintf(stderr, "[FAIL] %s: could not load script\n", testCase.name.c_str());
        return false;
    }

    FileLoaderContext loader;
    addUniquePath(loader.searchPaths, testCase.path.parent_path());
    addUniquePath(loader.searchPaths, testCase.path.parent_path() / "helpers");
    addUniquePath(loader.searchPaths, projectRoot / "tests" / "scripts");
    addUniquePath(loader.searchPaths, projectRoot / "tests" / "scripts" / "helpers");
    addUniquePath(loader.searchPaths, projectRoot);

    Interpreter vm;
    vm.registerAll();
    addUniquePath(loader.searchPaths, projectRoot / "bin");
    addUniquePath(loader.searchPaths, projectRoot / "bin" / "plugins");
    vm.setFileLoader(multiPathFileLoader, &loader);
    vm.addPluginSearchPath((projectRoot / "bin").string().c_str());
    vm.addPluginSearchPath((projectRoot / "bin" / "plugins").string().c_str());

    bool ok = false;
    try
    {
        ok = vm.run(source.c_str(), false);
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "[FAIL] %s: exception: %s\n", testCase.name.c_str(), e.what());
        return false;
    }
    catch (...)
    {
        std::fprintf(stderr, "[FAIL] %s: unknown exception\n", testCase.name.c_str());
        return false;
    }

    if (!ok)
    {
        std::fprintf(stderr, "[FAIL] %s: runtime returned false\n", testCase.name.c_str());
        return false;
    }

    Value passValue;
    if (!vm.tryGetGlobal("__test_pass", &passValue) || !passValue.isBool())
    {
        std::fprintf(stderr, "[FAIL] %s: missing bool global __test_pass\n", testCase.name.c_str());
        return false;
    }

    Value messageValue;
    std::string detail;
    if (vm.tryGetGlobal("__test_message", &messageValue) && messageValue.isString())
        detail = messageValue.asStringChars();

    if (!passValue.asBool())
    {
        std::fprintf(stderr, "[FAIL] %s", testCase.name.c_str());
        if (!detail.empty())
            std::fprintf(stderr, ": %s", detail.c_str());
        std::fprintf(stderr, "\n");
        return false;
    }

    std::printf("[PASS] %s", testCase.name.c_str());
    if (!detail.empty())
        std::printf(": %s", detail.c_str());
    std::printf("\n");
    return true;
}

int main(int argc, char *argv[])
{
    const fs::path projectRoot = resolveProjectRoot(argc > 0 ? argv[0] : nullptr);
    const fs::path testsRoot = projectRoot / "tests" / "scripts";

    std::vector<TestCase> tests;
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            const fs::path path = resolveInputPath(fs::path(argv[i]), projectRoot);
            tests.push_back({path.lexically_normal(), path.stem().string()});
        }
    }
    else
    {
        tests = discoverTests(testsRoot);
    }

    if (tests.empty())
    {
        std::fprintf(stderr, "No tests found in %s\n", testsRoot.string().c_str());
        return 1;
    }

    std::printf("Running %zu BuLang tests\n", tests.size());

    int passed = 0;
    for (const TestCase &testCase : tests)
    {
        if (runTest(projectRoot, testCase))
            passed++;
    }

    const int failed = static_cast<int>(tests.size()) - passed;
    std::printf("Summary: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
