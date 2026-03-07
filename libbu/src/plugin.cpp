
// ============================================
// Plugin System Implementation
// ============================================
#include "interpreter.hpp"
#include "platform.hpp"
#include "plugin.hpp"
#include <cstring>

static void setError(char* dest, size_t destSize, const char* msg)
{
    strncpy(dest, msg, destSize - 1);
    dest[destSize - 1] = '\0';
}

bool Interpreter::loadPlugin(const char *path)
{
    if (loadedPluginCount >= MAX_PLUGINS)
    {
        setError(lastPluginError, sizeof(lastPluginError), "Maximum plugins limit reached");
        return false;
    }

    // Try to load the library
    void* handle = OsLoadLibrary(path);
    if (!handle)
    {
        const char* err = OsGetLibraryError();
        setError(lastPluginError, sizeof(lastPluginError), err ? err : "Unknown error loading library");
        return false;
    }

    // Get the plugin info function
    BuGetPluginInfoFunc getInfo = (BuGetPluginInfoFunc)OsGetSymbol(handle, BU_PLUGIN_SYMBOL);
    if (!getInfo)
    {
        setError(lastPluginError, sizeof(lastPluginError), "Plugin does not export '" BU_PLUGIN_SYMBOL "' function");
        OsFreeLibrary(handle);
        return false;
    }

    // Get plugin info
    BuPluginInfo* info = getInfo();
    if (!info)
    {
        setError(lastPluginError, sizeof(lastPluginError), "Plugin returned null info");
        OsFreeLibrary(handle);
        return false;
    }

    // Check API version
    if (info->apiVersion != BU_PLUGIN_API_VERSION)
    {
        snprintf(lastPluginError, sizeof(lastPluginError),
                 "Plugin API version mismatch: expected %d, got %d",
                 BU_PLUGIN_API_VERSION, info->apiVersion);
        OsFreeLibrary(handle);
        return false;
    }

    // Check if module already exists
    if (containsModule(info->name))
    {
        snprintf(lastPluginError, sizeof(lastPluginError),
                 "Module '%s' already loaded", info->name);
        OsFreeLibrary(handle);
        return false;
    }

    // Register the module
    info->registerModule(this);

    // Track the loaded plugin
    LoadedPlugin& plugin = loadedPlugins[loadedPluginCount++];
    plugin.handle = handle;
    plugin.name = info->name;
    plugin.cleanup = info->cleanup;

    Info("Loaded plugin: %s v%s by %s", info->name, info->version, info->author);
    return true;
}

bool Interpreter::loadPluginByName(const char *name)
{
    char filename[MAX_PATH_LEN];
    char fullPath[MAX_PATH_LEN];

    // Build filename: libbu_<name>.so (or .dll/.dylib)
    snprintf(filename, sizeof(filename), "libbu_%s%s", name, OsGetLibraryExtension());

    // Try current directory first
    if (loadPlugin(filename))
        return true;

    // Try ./modules/
    snprintf(fullPath, sizeof(fullPath), "modules/%s", filename);
    if (loadPlugin(fullPath))
        return true;

    // Try each search path
    for (int i = 0; i < pluginSearchPathCount; i++)
    {
        const char* basePath = pluginSearchPaths[i];
        size_t len = strlen(basePath);

        if (len > 0 && basePath[len - 1] != '/' && basePath[len - 1] != '\\')
            snprintf(fullPath, sizeof(fullPath), "%s/%s", basePath, filename);
        else
            snprintf(fullPath, sizeof(fullPath), "%s%s", basePath, filename);

        if (loadPlugin(fullPath))
            return true;
    }

    snprintf(lastPluginError, sizeof(lastPluginError),
             "Could not find plugin '%s' (%s)", name, filename);
    return false;
}

void Interpreter::addPluginSearchPath(const char *path)
{
    if (pluginSearchPathCount >= MAX_PLUGIN_PATHS)
    {
        Warning("Maximum plugin search paths reached");
        return;
    }

    strncpy(pluginSearchPaths[pluginSearchPathCount], path, MAX_PATH_LEN - 1);
    pluginSearchPaths[pluginSearchPathCount][MAX_PATH_LEN - 1] = '\0';
    pluginSearchPathCount++;
}

void Interpreter::unloadAllPlugins()
{
    Info("Unloading all plugins");
    // Call cleanup functions and free libraries in reverse order
    for (int i = loadedPluginCount - 1; i >= 0; i--)
    {
        LoadedPlugin& plugin = loadedPlugins[i];
        if (plugin.cleanup)
        {
            plugin.cleanup();
        }
        OsFreeLibrary(plugin.handle);
    }
    loadedPluginCount = 0;
}

const char* Interpreter::getLastPluginError() const
{
    return lastPluginError;
}