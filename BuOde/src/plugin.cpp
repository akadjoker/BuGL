
#include "plugin.hpp"
#include "bindings.hpp"

// Plugin registration function
void register_ode_module(Interpreter* vm)
{
    ODEBindings::registerAll(*vm);
}

// Plugin cleanup function (optional)
void cleanup_ode_module()
{
    // ODE cleanup is handled by ODE in user code
}

// Export the plugin
BU_DEFINE_PLUGIN("ODE", "1.0.0", "BuLang", register_ode_module, cleanup_ode_module)