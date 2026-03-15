#include "plugin.hpp"
#include "bindings.hpp"

void register_micropather_module(Interpreter *vm)
{
    MicroPatherBindings::registerAll(*vm);
}

void cleanup_micropather_module()
{
    MicroPatherBindings::cleanup();
}

BU_DEFINE_PLUGIN("MicroPather", "1.0.0", "BuLang", register_micropather_module, cleanup_micropather_module)
