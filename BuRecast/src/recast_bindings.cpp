#include "recast_core.hpp"

namespace RecastBindings
{
    void registerAll(Interpreter &vm)
    {
        g_vector3Def = get_native_struct_def(&vm, "Vector3");

        register_navmesh(vm);
        register_crowd(vm);
        register_tilecache(vm);

        ModuleBuilder module = vm.addModule("Recast");
        module.addInt("OBSTACLE_EMPTY", (int)DT_OBSTACLE_EMPTY)
              .addInt("OBSTACLE_PROCESSING", (int)DT_OBSTACLE_PROCESSING)
              .addInt("OBSTACLE_PROCESSED", (int)DT_OBSTACLE_PROCESSED)
              .addInt("OBSTACLE_REMOVING", (int)DT_OBSTACLE_REMOVING)
              .addInt("OBSTACLE_CYLINDER", (int)DT_OBSTACLE_CYLINDER)
              .addInt("OBSTACLE_BOX", (int)DT_OBSTACLE_BOX)
              .addInt("OBSTACLE_ORIENTED_BOX", (int)DT_OBSTACLE_ORIENTED_BOX);
    }

    void cleanup()
    {
        g_vector3Def    = nullptr;
        g_navMeshClass  = nullptr;
        g_navCrowdClass = nullptr;
        g_navTileCacheClass = nullptr;
    }
}
