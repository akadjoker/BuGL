#pragma once

#include "interpreter.hpp"

// Plugin API version - increment when breaking changes are made
#define BU_PLUGIN_API_VERSION 1

// Plugin info structure that each plugin must export
struct BuPluginInfo {
    int apiVersion;                            // Must match BU_PLUGIN_API_VERSION
    const char* name;                          // Module name (e.g., "raylib", "SDL")
    const char* version;                       // Plugin version string
    const char* author;                        // Plugin author
    void (*registerModule)(Interpreter* vm);   // Function to register the module
    void (*cleanup)();                         // Optional cleanup function (can be nullptr)
};

// Function signature that plugins must export
typedef BuPluginInfo* (*BuGetPluginInfoFunc)();

// Symbol name that will be looked up when loading plugins
#define BU_PLUGIN_SYMBOL "bu_get_plugin_info"

// Macro to easily define a plugin
// Usage:
//   void my_register(Interpreter* vm) { ... }
//   void my_cleanup() { ... }  // optional, can be nullptr
//   BU_DEFINE_PLUGIN("mymodule", "1.0", "Author Name", my_register, my_cleanup);
//
#define BU_DEFINE_PLUGIN(moduleName, moduleVersion, moduleAuthor, registerFunc, cleanupFunc) \
    extern "C" BuPluginInfo* bu_get_plugin_info() {                                          \
        static BuPluginInfo info = {                                                          \
            BU_PLUGIN_API_VERSION,                                                            \
            moduleName,                                                                       \
            moduleVersion,                                                                    \
            moduleAuthor,                                                                     \
            registerFunc,                                                                     \
            cleanupFunc                                                                       \
        };                                                                                    \
        return &info;                                                                         \
    }

// Helper macro for plugins that don't need cleanup
#define BU_DEFINE_PLUGIN_SIMPLE(moduleName, moduleVersion, moduleAuthor, registerFunc) \
    BU_DEFINE_PLUGIN(moduleName, moduleVersion, moduleAuthor, registerFunc, nullptr)
