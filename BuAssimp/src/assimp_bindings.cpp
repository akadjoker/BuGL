#include "assimp_core.hpp"

namespace AssimpBindings
{
    void registerAll(Interpreter &vm)
    {
        register_mesh(vm);
        register_material(vm);
        register_node(vm);
        register_animation(vm);
        register_scene(vm);

        g_matrixDef = get_native_struct_def(&vm, "Matrix");

        ModuleBuilder module = vm.addModule("Assimp");


        module.addInt("CalcTangentSpace",          (int)aiProcess_CalcTangentSpace)
              .addInt("JoinIdenticalVertices",     (int)aiProcess_JoinIdenticalVertices)
              .addInt("MakeLeftHanded",            (int)aiProcess_MakeLeftHanded)
              .addInt("Triangulate",               (int)aiProcess_Triangulate)
              .addInt("RemoveComponent",           (int)aiProcess_RemoveComponent)
              .addInt("GenNormals",                (int)aiProcess_GenNormals)
              .addInt("GenSmoothNormals",          (int)aiProcess_GenSmoothNormals)
              .addInt("SplitLargeMeshes",          (int)aiProcess_SplitLargeMeshes)
              .addInt("PreTransformVertices",      (int)aiProcess_PreTransformVertices)
              .addInt("LimitBoneWeights",          (int)aiProcess_LimitBoneWeights)
              .addInt("ValidateDataStructure",     (int)aiProcess_ValidateDataStructure)
              .addInt("ImproveCacheLocality",      (int)aiProcess_ImproveCacheLocality)
              .addInt("RemoveRedundantMaterials",  (int)aiProcess_RemoveRedundantMaterials)
              .addInt("FixInfacingNormals",        (int)aiProcess_FixInfacingNormals)
              .addInt("PopulateArmatureData",      (int)aiProcess_PopulateArmatureData)
              .addInt("SortByPType",               (int)aiProcess_SortByPType)
              .addInt("FindDegenerates",           (int)aiProcess_FindDegenerates)
              .addInt("FindInvalidData",           (int)aiProcess_FindInvalidData)
              .addInt("GenUVCoords",               (int)aiProcess_GenUVCoords)
              .addInt("TransformUVCoords",         (int)aiProcess_TransformUVCoords)
              .addInt("FindInstances",             (int)aiProcess_FindInstances)
              .addInt("OptimizeMeshes",            (int)aiProcess_OptimizeMeshes)
              .addInt("OptimizeGraph",             (int)aiProcess_OptimizeGraph)
              .addInt("FlipUVs",                   (int)aiProcess_FlipUVs)
              .addInt("FlipWindingOrder",          (int)aiProcess_FlipWindingOrder)
              .addInt("SplitByBoneCount",          (int)aiProcess_SplitByBoneCount)
              .addInt("Debone",                    (int)aiProcess_Debone)
              .addInt("GlobalScale",               (int)aiProcess_GlobalScale)
              .addInt("EmbedTextures",             (int)aiProcess_EmbedTextures)
              .addInt("ForceGenNormals",           (int)aiProcess_ForceGenNormals)
              .addInt("DropNormals",               (int)aiProcess_DropNormals)
              .addInt("GenBoundingBoxes",          (int)aiProcess_GenBoundingBoxes)
        // Presets
              .addInt("Preset_Fast",               (int)aiProcessPreset_TargetRealtime_Fast)
              .addInt("Preset_Quality",            (int)aiProcessPreset_TargetRealtime_Quality)
              .addInt("Preset_MaxQuality",         (int)aiProcessPreset_TargetRealtime_MaxQuality)
              .addInt("Preset_D3D",                (int)aiProcess_ConvertToLeftHanded)
        // TextureType -- todos incluindo PBR
              .addInt("TextureNone",               (int)aiTextureType_NONE)
              .addInt("TextureDiffuse",            (int)aiTextureType_DIFFUSE)
              .addInt("TextureSpecular",           (int)aiTextureType_SPECULAR)
              .addInt("TextureAmbient",            (int)aiTextureType_AMBIENT)
              .addInt("TextureEmissive",           (int)aiTextureType_EMISSIVE)
              .addInt("TextureHeight",             (int)aiTextureType_HEIGHT)
              .addInt("TextureNormals",            (int)aiTextureType_NORMALS)
              .addInt("TextureShininess",          (int)aiTextureType_SHININESS)
              .addInt("TextureOpacity",            (int)aiTextureType_OPACITY)
              .addInt("TextureDisplacement",       (int)aiTextureType_DISPLACEMENT)
              .addInt("TextureLightmap",           (int)aiTextureType_LIGHTMAP)
              .addInt("TextureReflection",         (int)aiTextureType_REFLECTION)
              .addInt("TextureBaseColor",          (int)aiTextureType_BASE_COLOR)
              .addInt("TextureNormalCamera",       (int)aiTextureType_NORMAL_CAMERA)
              .addInt("TextureEmissionColor",      (int)aiTextureType_EMISSION_COLOR)
              .addInt("TextureMetalness",          (int)aiTextureType_METALNESS)
              .addInt("TextureDiffuseRoughness",   (int)aiTextureType_DIFFUSE_ROUGHNESS)
              .addInt("TextureAmbientOcclusion",   (int)aiTextureType_AMBIENT_OCCLUSION)
              .addInt("TextureUnknown",            (int)aiTextureType_UNKNOWN)
        // Animation behaviour
              .addInt("AnimDefault",               (int)aiAnimBehaviour_DEFAULT)
              .addInt("AnimConstant",              (int)aiAnimBehaviour_CONSTANT)
              .addInt("AnimLinear",                (int)aiAnimBehaviour_LINEAR)
              .addInt("AnimRepeat",                (int)aiAnimBehaviour_REPEAT);
    }

    void cleanup()
    {
        g_sceneClass    = nullptr;
        g_meshClass     = nullptr;
        g_materialClass = nullptr;
        g_nodeClass     = nullptr;
        g_animationClass = nullptr;
        g_nodeAnimClass  = nullptr;
        g_matrixDef     = nullptr;
    }
}
