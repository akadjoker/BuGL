#include "plugin.hpp"
#include "bindings.hpp"

void register_jolt_module(Interpreter *vm)
{
    JoltBindings::registerAll(*vm);
}

void cleanup_jolt_module()
{
    JoltBindings::cleanup();
}

BU_DEFINE_PLUGIN("Jolt", "1.0.0", "BuLang", register_jolt_module, cleanup_jolt_module)
