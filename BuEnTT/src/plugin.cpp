#include "plugin.hpp"
#include "bindings.hpp"

void register_entt_module(Interpreter *vm)
{
    EnTTBindings::registerAll(*vm);
}

void cleanup_entt_module()
{
    EnTTBindings::cleanup();
}

BU_DEFINE_PLUGIN("EnTT", "1.0.0", "BuLang", register_entt_module, cleanup_entt_module)
