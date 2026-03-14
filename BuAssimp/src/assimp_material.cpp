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
        if (!h) return;
        h->owner->release();
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
        if (argCount > 0 && args[0].isNumber()) type = (int)args[0].asDouble();
        unsigned int count = h->mat->GetTextureCount((aiTextureType)type);
        vm->pushInt((int)count);
        return 1;
    }

    // getTexturePath(type [, index=0]) -> string
    static int material_get_texture_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpMaterial.getTexturePath(type [, index])"); return push_nil1(vm); }
        MaterialHandle *h = require_material(instance, "AssimpMaterial.getTexturePath()");
        if (!h) return push_nil1(vm);
        int type  = (int)args[0].asDouble();
        int index = (argCount > 1 && args[1].isNumber()) ? (int)args[1].asDouble() : 0;
        aiString path;
        if (h->mat->GetTexture((aiTextureType)type, (unsigned int)index, &path) != AI_SUCCESS)
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
        vm.addNativeMethod(g_materialClass, "getTexturePath",    material_get_texture_path);
    }

} // namespace AssimpBindings
