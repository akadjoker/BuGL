#include "plugin.hpp"
#include "bindings.hpp"

void register_chip2d_module(Interpreter *vm)
{
    ChipmunkBindings::registerAll(*vm);
}

void cleanup_chip2d_module()
{
}

BU_DEFINE_PLUGIN("Chip2D", "1.0.0", "BuLang", register_chip2d_module, cleanup_chip2d_module)
