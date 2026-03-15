#include "opensteer_core.hpp"

namespace OpenSteerBindings
{
    void registerAll(Interpreter &vm)
    {
        g_vector3Def = get_native_struct_def(&vm, "Vector3");

        register_agent(vm);
        register_obstacle(vm);
        register_pathway(vm);

        ModuleBuilder module = vm.addModule("OpenSteer");
        module.addInt("SEEN_FROM_OUTSIDE", (int)OpenSteer::AbstractObstacle::outside)
              .addInt("SEEN_FROM_INSIDE", (int)OpenSteer::AbstractObstacle::inside)
              .addInt("SEEN_FROM_BOTH", (int)OpenSteer::AbstractObstacle::both);
    }

    void cleanup()
    {
        g_vector3Def = nullptr;
        g_agentClass = nullptr;
        g_sphereObstacleClass = nullptr;
        g_pathwayClass = nullptr;
    }
}
