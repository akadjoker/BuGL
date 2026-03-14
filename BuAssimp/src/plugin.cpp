#include "plugin.hpp"
#include "assimp_bindings.hpp"

void register_assimp_module(Interpreter *vm)
{
    AssimpBindings::registerAll(*vm);
}

void cleanup_assimp_module()
{
    AssimpBindings::cleanup();
}

BU_DEFINE_PLUGIN("Assimp", "1.0.0", "BuLang", register_assimp_module, cleanup_assimp_module)
