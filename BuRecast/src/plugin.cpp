#include "plugin.hpp"
#include "recast_bindings.hpp"

void register_recast_module(Interpreter *vm)
{
    RecastBindings::registerAll(*vm);
}

void cleanup_recast_module()
{
    RecastBindings::cleanup();
}

BU_DEFINE_PLUGIN("Recast", "1.0.0", "BuLang", register_recast_module, cleanup_recast_module)
