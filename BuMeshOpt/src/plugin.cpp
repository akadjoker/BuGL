#include "plugin.hpp"
#include "bindings.hpp"

void register_meshopt_module(Interpreter *vm)
{
    MeshOptBindings::registerAll(*vm);
}

void cleanup_meshopt_module()
{
    MeshOptBindings::cleanup();
}

BU_DEFINE_PLUGIN("MeshOptimizer", "1.0.0", "BuLang", register_meshopt_module, cleanup_meshopt_module)
