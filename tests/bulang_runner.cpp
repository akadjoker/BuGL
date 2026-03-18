// ============================================
// Minimal BuLang test runner
// Only depends on libbu — no SDL, OpenGL, or plugins.
// Usage: bulang_test <script.bu> [--dump]
// Exit code 0 = success, 1 = error
// ============================================

#include "interpreter.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// ── Simple file loader ──────────────────────────────────────

struct SimpleLoaderCtx
{
    std::vector<std::string> searchPaths;
    std::string buffer; // keeps data alive between calls
};

static std::string readWholeFile(const char *path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
        return {};
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static const char *simpleFileLoader(const char *filename, size_t *outSize, void *userdata)
{
    if (!filename || !outSize || !userdata)
        return nullptr;

    auto *ctx = static_cast<SimpleLoaderCtx *>(userdata);
    *outSize = 0;

    // Strip leading slashes for relative resolution
    const char *rel = filename;
    while (*rel == '/' || *rel == '\\')
        ++rel;

    auto tryLoad = [&](const std::string &path) -> bool
    {
        std::string data = readWholeFile(path.c_str());
        if (data.empty())
            return false;
        ctx->buffer = std::move(data);
        *outSize = ctx->buffer.size();
        return true;
    };

    // Try search paths
    for (const auto &dir : ctx->searchPaths)
    {
        if (tryLoad(dir + "/" + rel))
            return ctx->buffer.c_str();
    }

    // Fallback: try as-is
    if (tryLoad(filename))
        return ctx->buffer.c_str();
    if (rel != filename && tryLoad(rel))
        return ctx->buffer.c_str();

    return nullptr;
}

// ── dirname helper ──────────────────────────────────────────

static std::string dirnameof(const std::string &path)
{
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
        return ".";
    return path.substr(0, pos);
}

// ── main ────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    const char *scriptFile = nullptr;
    bool dump = false;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--dump") == 0)
        {
            dump = true;
        }
        else if (!scriptFile)
        {
            scriptFile = argv[i];
        }
    }

    if (!scriptFile)
    {
        std::fprintf(stderr, "Usage: bulang_test <script.bu> [--dump]\n");
        return 1;
    }

    // Load source
    std::string source = readWholeFile(scriptFile);
    if (source.empty())
    {
        std::fprintf(stderr, "Error: cannot open '%s'\n", scriptFile);
        return 1;
    }

    // Set up VM
    Interpreter vm;
    vm.registerAll();

    // Configure file loader with search paths relative to the script
    SimpleLoaderCtx loaderCtx;
    std::string scriptDir = dirnameof(scriptFile);
    loaderCtx.searchPaths.push_back(scriptDir);
    loaderCtx.searchPaths.push_back(scriptDir + "/..");
    loaderCtx.searchPaths.push_back("scripts");
    loaderCtx.searchPaths.push_back(".");
    vm.setFileLoader(simpleFileLoader, &loaderCtx);

    // Run
    bool ok = vm.run(source.c_str(), dump);
    if (!ok)
    {
        std::fprintf(stderr, "FAIL: %s\n", scriptFile);
        return 1;
    }

    return 0;
}
