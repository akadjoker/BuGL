#include "entt_core.hpp"

namespace EnTTBindings
{
    void registerAll(Interpreter &vm)
    {
        register_entity(vm);
        register_world(vm);

        ModuleBuilder module = vm.addModule("EnTT");
        module.addInt("NullEntity", -1)
              .addInt("ComponentTag", 1)
              .addInt("ComponentTransform2D", 2)
              .addInt("ComponentVelocity2D", 3)
              .addInt("ComponentTransform3D", 4);
    }
}
