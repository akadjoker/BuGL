#include "bindings_internal.hpp"

namespace ImGuiBindings
{
    void registerAll(Interpreter &vm)
    {
        ModuleBuilder module = vm.addModule("ImGui");

        register_core(module);
        register_layout(module);
        register_style(module);
        register_text(module);
        register_inputs(module);
    }
}
