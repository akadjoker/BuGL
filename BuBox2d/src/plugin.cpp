
#include "plugin.hpp"
#include "bindings.hpp"

// Plugin registration function
void register_box2d_module(Interpreter* vm)
{
    BOX2DBindings::register_box2d(*vm);
    BOX2DBindings::register_box2d_joints(*vm);
}

// Plugin cleanup function (optional)
void cleanup_box2d_module()
{
    // Box2D cleanup is handled by Box2D in user code
}

// Export the plugin
BU_DEFINE_PLUGIN("Box2D", "1.0.0", "BuLang", register_box2d_module, cleanup_box2d_module)
