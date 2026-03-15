#include "micropather_core.hpp"

namespace MicroPatherBindings
{
    void registerAll(Interpreter &vm)
    {
        register_grid(vm);

        ModuleBuilder module = vm.addModule("MicroPather");
        module.addInt("SOLVED", micropather::MicroPather::SOLVED)
              .addInt("NO_SOLUTION", micropather::MicroPather::NO_SOLUTION)
              .addInt("START_END_SAME", micropather::MicroPather::START_END_SAME);
    }

    void cleanup()
    {
        g_gridPathfinderClass = nullptr;
    }
}
