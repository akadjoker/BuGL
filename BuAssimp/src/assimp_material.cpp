#include "assimp_core.hpp"
#include <assimp/material.h>

namespace AssimpBindings
{
    static void *material_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm; (void)argCount; (void)args;
        Error("AssimpMaterial cannot be constructed directly; use AssimpScene.getMaterial()");
        return nullptr;
    }

    static void material_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        MaterialHandle *h = (MaterialHandle *)instance;
        delete h;
    }

    static int material_get_name(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getName()");
        if (!h) return push_nil1(vm);
        aiString name;
        h->mat->Get(AI_MATKEY_NAME, name);
        vm->pushString(name.C_Str());
        return 1;
    }

    // getDiffuseColor() -> r, g, b, a
    static int material_get_diffuse(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getDiffuseColor()");
        if (!h) return push_nil1(vm);
        aiColor4D c(1.0f, 1.0f, 1.0f, 1.0f);
        h->mat->Get(AI_MATKEY_COLOR_DIFFUSE, c);
        vm->pushDouble((double)c.r);
        vm->pushDouble((double)c.g);
        vm->pushDouble((double)c.b);
        vm->pushDouble((double)c.a);
        return 4;
    }

    // getSpecularColor() -> r, g, b, a
    static int material_get_specular(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getSpecularColor()");
        if (!h) return push_nil1(vm);
        aiColor4D c(0.0f, 0.0f, 0.0f, 1.0f);
        h->mat->Get(AI_MATKEY_COLOR_SPECULAR, c);
        vm->pushDouble((double)c.r);
        vm->pushDouble((double)c.g);
        vm->pushDouble((double)c.b);
        vm->pushDouble((double)c.a);
        return 4;
    }

    // getAmbientColor() -> r, g, b, a
    static int material_get_ambient(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getAmbientColor()");
        if (!h) return push_nil1(vm);
        aiColor4D c(0.0f, 0.0f, 0.0f, 1.0f);
        h->mat->Get(AI_MATKEY_COLOR_AMBIENT, c);
        vm->pushDouble((double)c.r);
        vm->pushDouble((double)c.g);
        vm->pushDouble((double)c.b);
        vm->pushDouble((double)c.a);
        return 4;
    }

    // getEmissiveColor() -> r, g, b, a
    static int material_get_emissive(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getEmissiveColor()");
        if (!h) return push_nil1(vm);
        aiColor4D c(0.0f, 0.0f, 0.0f, 1.0f);
        h->mat->Get(AI_MATKEY_COLOR_EMISSIVE, c);
        vm->pushDouble((double)c.r);
        vm->pushDouble((double)c.g);
        vm->pushDouble((double)c.b);
        vm->pushDouble((double)c.a);
        return 4;
    }

    // getOpacity() -> float
    static int material_get_opacity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getOpacity()");
        if (!h) return push_nil1(vm);
        float val = 1.0f;
        h->mat->Get(AI_MATKEY_OPACITY, val);
        vm->pushDouble((double)val);
        return 1;
    }

    // getShininess() -> float
    static int material_get_shininess(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getShininess()");
        if (!h) return push_nil1(vm);
        float val = 0.0f;
        h->mat->Get(AI_MATKEY_SHININESS, val);
        vm->pushDouble((double)val);
        return 1;
    }

    // getTextureCount(type) -> int
    // type: 0=none,1=diffuse,2=specular,3=ambient,4=emissive,5=height,6=normals,7=shininess,8=opacity,9=displacement,10=lightmap,11=reflection
    static int material_get_texture_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getTextureCount()");
        if (!h) return push_nil1(vm);
        int type = 1; // default: diffuse
        if (argCount > 0)
        {
            double typed = 0.0;
            if (!read_number_arg(args[0], &typed, "AssimpMaterial.getTextureCount()", 1))
                return push_nil1(vm);
            type = (int)typed;
        }
        unsigned int count = h->mat->GetTextureCount((aiTextureType)type);
        vm->pushInt((int)count);
        return 1;
    }

    // hasTexture(type [, index=0]) -> bool
    static int material_has_texture(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpMaterial.hasTexture(type [, index])"); return push_nil1(vm); }
        MaterialHandle *h = require_material(instance, "AssimpMaterial.hasTexture()");
        if (!h) return push_nil1(vm);

        double typeD = 0.0;
        if (!read_number_arg(args[0], &typeD, "AssimpMaterial.hasTexture()", 1))
            return push_nil1(vm);

        int index = 0;
        if (argCount > 1)
        {
            double idxD = 0.0;
            if (!read_number_arg(args[1], &idxD, "AssimpMaterial.hasTexture()", 2))
                return push_nil1(vm);
            index = (int)idxD;
        }

        aiString path;
        bool ok = (h->mat->GetTexture((aiTextureType)(int)typeD, (unsigned int)index, &path) == AI_SUCCESS);
        vm->pushBool(ok);
        return 1;
    }

    // getTexturePath(type [, index=0]) -> string
    static int material_get_texture_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpMaterial.getTexturePath(type [, index])"); return push_nil1(vm); }
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getTexturePath()");
        if (!h) return push_nil1(vm);

        double typeD = 0.0;
        if (!read_number_arg(args[0], &typeD, "AssimpMaterial.getTexturePath()", 1))
            return push_nil1(vm);

        int index = 0;
        if (argCount > 1)
        {
            double idxD = 0.0;
            if (!read_number_arg(args[1], &idxD, "AssimpMaterial.getTexturePath()", 2))
                return push_nil1(vm);
            index = (int)idxD;
        }

        int type = (int)typeD;
        aiString path;
        if (h->mat->GetTexture((aiTextureType)type, (unsigned int)index, &path) != AI_SUCCESS)
        {
            vm->pushString("");
            return 1;
        }
        vm->pushString(path.C_Str());
        return 1;
    }

    // getFirstTexturePath(type) -> string
    static int material_get_first_texture_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpMaterial.getFirstTexturePath(type)"); return push_nil1(vm); }
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getFirstTexturePath()");
        if (!h) return push_nil1(vm);

        double typeD = 0.0;
        if (!read_number_arg(args[0], &typeD, "AssimpMaterial.getFirstTexturePath()", 1))
            return push_nil1(vm);

        aiString path;
        if (h->mat->GetTexture((aiTextureType)(int)typeD, 0u, &path) != AI_SUCCESS)
        {
            vm->pushString("");
            return 1;
        }

        vm->pushString(path.C_Str());
        return 1;
    }

    // getTexturePaths(type) -> array<string>
    static int material_get_texture_paths(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpMaterial.getTexturePaths(type)"); return push_nil1(vm); }
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getTexturePaths()");
        if (!h) return push_nil1(vm);

        double typeD = 0.0;
        if (!read_number_arg(args[0], &typeD, "AssimpMaterial.getTexturePaths()", 1))
            return push_nil1(vm);

        aiTextureType type = (aiTextureType)(int)typeD;
        unsigned int count = h->mat->GetTextureCount(type);

        Value arr = vm->makeArray();
        ArrayInstance *a = arr.as.array;
        a->values.reserve(count);
        for (unsigned int i = 0; i < count; ++i)
        {
            aiString path;
            if (h->mat->GetTexture(type, i, &path) == AI_SUCCESS)
                a->values.push(vm->makeString(path.C_Str()));
            else
                a->values.push(vm->makeString(""));
        }

        vm->push(arr);
        return 1;
    }

    static int material_get_diffuse_texture_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getDiffuseTexturePath()");
        if (!h) return push_nil1(vm);

        int index = 0;
        if (argCount > 0)
        {
            double idxD = 0.0;
            if (!read_number_arg(args[0], &idxD, "AssimpMaterial.getDiffuseTexturePath()", 1))
                return push_nil1(vm);
            index = (int)idxD;
        }

        aiString path;
        if (h->mat->GetTexture(aiTextureType_DIFFUSE, (unsigned int)index, &path) != AI_SUCCESS)
        {
            vm->pushString("");
            return 1;
        }

        vm->pushString(path.C_Str());
        return 1;
    }

    static int material_get_normal_texture_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getNormalTexturePath()");
        if (!h) return push_nil1(vm);

        int index = 0;
        if (argCount > 0)
        {
            double idxD = 0.0;
            if (!read_number_arg(args[0], &idxD, "AssimpMaterial.getNormalTexturePath()", 1))
                return push_nil1(vm);
            index = (int)idxD;
        }

        aiString path;
        if (h->mat->GetTexture(aiTextureType_NORMALS, (unsigned int)index, &path) != AI_SUCCESS)
        {
            vm->pushString("");
            return 1;
        }

        vm->pushString(path.C_Str());
        return 1;
    }

    static int material_get_emissive_texture_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getEmissiveTexturePath()");
        if (!h) return push_nil1(vm);

        int index = 0;
        if (argCount > 0)
        {
            double idxD = 0.0;
            if (!read_number_arg(args[0], &idxD, "AssimpMaterial.getEmissiveTexturePath()", 1))
                return push_nil1(vm);
            index = (int)idxD;
        }

        aiString path;
        if (h->mat->GetTexture(aiTextureType_EMISSIVE, (unsigned int)index, &path) != AI_SUCCESS)
        {
            vm->pushString("");
            return 1;
        }

        vm->pushString(path.C_Str());
        return 1;
    }

    void register_material(Interpreter &vm)
    {
        g_materialClass = vm.registerNativeClass("AssimpMaterial", material_ctor_error, material_dtor, 0, false);
        vm.addNativeMethod(g_materialClass, "getName",           material_get_name);
        vm.addNativeMethod(g_materialClass, "getDiffuseColor",   material_get_diffuse);
        vm.addNativeMethod(g_materialClass, "getSpecularColor",  material_get_specular);
        vm.addNativeMethod(g_materialClass, "getAmbientColor",   material_get_ambient);
        vm.addNativeMethod(g_materialClass, "getEmissiveColor",  material_get_emissive);
        vm.addNativeMethod(g_materialClass, "getOpacity",        material_get_opacity);
        vm.addNativeMethod(g_materialClass, "getShininess",      material_get_shininess);
        vm.addNativeMethod(g_materialClass, "getTextureCount",   material_get_texture_count);
        vm.addNativeMethod(g_materialClass, "hasTexture",        material_has_texture);
        vm.addNativeMethod(g_materialClass, "getTexturePath",    material_get_texture_path);
        vm.addNativeMethod(g_materialClass, "getFirstTexturePath", material_get_first_texture_path);
        vm.addNativeMethod(g_materialClass, "getTexturePaths",   material_get_texture_paths);
        vm.addNativeMethod(g_materialClass, "getDiffuseTexturePath", material_get_diffuse_texture_path);
        vm.addNativeMethod(g_materialClass, "getNormalTexturePath", material_get_normal_texture_path);
        vm.addNativeMethod(g_materialClass, "getEmissiveTexturePath", material_get_emissive_texture_path);
    }

} // namespace AssimpBindings
