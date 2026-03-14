#include "plugin.hpp"
#include "bindings.hpp"

void register_opensteer_module(Interpreter *vm)
{
    OpenSteerBindings::registerAll(*vm);
}

void cleanup_opensteer_module()
{
    OpenSteerBindings::cleanup();
}

BU_DEFINE_PLUGIN("OpenSteer", "1.0.0", "BuLang", register_opensteer_module, cleanup_opensteer_module)
