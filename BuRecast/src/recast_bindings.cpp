#include "recast_core.hpp"

namespace RecastBindings
{
    void registerAll(Interpreter &vm)
    {
        g_vector3Def = get_native_struct_def(&vm, "Vector3");

        register_navmesh(vm);
        register_crowd(vm);

        ModuleBuilder module = vm.addModule("Recast");
        (void)module; // no module-level constants for now
    }

    void cleanup()
    {
        g_vector3Def    = nullptr;
        g_navMeshClass  = nullptr;
        g_navCrowdClass = nullptr;
    }
}
