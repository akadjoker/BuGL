#include "assimp_core.hpp"

namespace AssimpBindings
{
    void registerAll(Interpreter &vm)
    {
       
        register_mesh(vm);
        register_material(vm);
        register_node(vm);
        register_scene(vm);

        ModuleBuilder module = vm.addModule("Assimp");

        // aiProcess_* postprocess flags
        module.addInt("Triangulate",           0x8);
        module.addInt("JoinIdenticalVertices", 0x2);
        module.addInt("CalcTangentSpace",      0x1);
        module.addInt("GenNormals",            0x20);
        module.addInt("GenSmoothNormals",      0x40);
        module.addInt("PreTransformVertices",  0x100);
        module.addInt("FlipUVs",               0x800000);
        module.addInt("OptimizeMeshes",        0x200000);
        module.addInt("SortByPType",           0x8000);

        // aiTextureType constants
        module.addInt("TextureNone",        0);
        module.addInt("TextureDiffuse",     1);
        module.addInt("TextureSpecular",    2);
        module.addInt("TextureAmbient",     3);
        module.addInt("TextureEmissive",    4);
        module.addInt("TextureHeight",      5);
        module.addInt("TextureNormals",     6);
        module.addInt("TextureShininess",   7);
        module.addInt("TextureOpacity",     8);
        module.addInt("TextureDisplacement",9);
        module.addInt("TextureLightmap",    10);
        module.addInt("TextureReflection",  11);
    }

    void cleanup()
    {
        g_sceneClass    = nullptr;
        g_meshClass     = nullptr;
        g_materialClass = nullptr;
        g_nodeClass     = nullptr;
    }
}
