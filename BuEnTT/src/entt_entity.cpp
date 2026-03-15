#include "entt_core.hpp"

#include <cstring>

namespace EnTTBindings
{
    enum class ComponentKind
    {
        Unknown = 0,
        Tag,
        Transform2D,
        Velocity2D,
        Transform3D
    };

    static ComponentKind parse_component_kind(const char *name)
    {
        if (!name)
            return ComponentKind::Unknown;

        if (std::strcmp(name, "Tag") == 0 || std::strcmp(name, "tag") == 0)
            return ComponentKind::Tag;

        if (std::strcmp(name, "Transform2D") == 0 || std::strcmp(name, "transform2d") == 0)
            return ComponentKind::Transform2D;

        if (std::strcmp(name, "Velocity2D") == 0 || std::strcmp(name, "velocity2d") == 0)
            return ComponentKind::Velocity2D;

        if (std::strcmp(name, "Transform3D") == 0 || std::strcmp(name, "transform3d") == 0)
            return ComponentKind::Transform3D;

        return ComponentKind::Unknown;
    }

    // ctor/dtor lives in entt_core.cpp
    void *entt_entity_ctor(Interpreter *vm, int argCount, Value *args);
    void entt_entity_dtor(Interpreter *vm, void *instance);

    static Transform2DComponent read_or_default_transform(WorldRuntime *runtime, entt::entity entity)
    {
        if (!runtime || entity == entt::null)
            return Transform2DComponent{};

        Transform2DComponent *existing = runtime->registry.try_get<Transform2DComponent>(entity);
        if (existing)
            return *existing;

        return Transform2DComponent{};
    }

    static Velocity2DComponent read_or_default_velocity(WorldRuntime *runtime, entt::entity entity)
    {
        if (!runtime || entity == entt::null)
            return Velocity2DComponent{};

        Velocity2DComponent *existing = runtime->registry.try_get<Velocity2DComponent>(entity);
        if (existing)
            return *existing;

        return Velocity2DComponent{};
    }

    static Transform3DComponent read_or_default_transform3d(WorldRuntime *runtime, entt::entity entity)
    {
        if (!runtime || entity == entt::null)
            return Transform3DComponent{};

        Transform3DComponent *existing = runtime->registry.try_get<Transform3DComponent>(entity);
        if (existing)
            return *existing;

        return Transform3DComponent{};
    }

    static int entity_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.destroy() expects 0 arguments");
            return 0;
        }

        EntityHandle *entity = (EntityHandle *)instance;
        if (!entity)
            return 0;

        WorldRuntime *runtime = find_world_runtime(entity->worldId);
        if (runtime && entity->valid && entity->entity != entt::null && runtime->registry.valid(entity->entity))
            runtime->registry.destroy(entity->entity);

        entity->entity = entt::null;
        entity->valid = false;
        return 0;
    }

    static int entity_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        EntityHandle *entity = (EntityHandle *)instance;
        vm->pushBool(is_entity_alive(entity));
        return 1;
    }

    static int entity_get_id(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getId() expects 0 arguments");
            return push_nil1(vm);
        }

        EntityHandle *entity = (EntityHandle *)instance;
        if (!entity || entity->entity == entt::null)
            return push_nil1(vm);

        vm->pushInt(entity_to_id(entity->entity));
        return 1;
    }

    static int entity_has_tag(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.hasTag() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.hasTag()", &runtime);
        if (!entity)
            return push_nil1(vm);

        vm->pushBool(runtime->registry.all_of<TagComponent>(entity->entity));
        return 1;
    }

    static int entity_set_tag(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSEntity.setTag(tag)");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setTag()", &runtime);
        if (!entity)
            return 0;

        const char *tag = nullptr;
        if (!read_string_arg(args[0], &tag, "ECSEntity.setTag()", 1))
            return 0;

        runtime->registry.emplace_or_replace<TagComponent>(entity->entity, TagComponent{tag ? tag : ""});
        return 0;
    }

    static int entity_get_tag(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getTag() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getTag()", &runtime);
        if (!entity)
            return push_nil1(vm);

        TagComponent *tag = runtime->registry.try_get<TagComponent>(entity->entity);
        if (!tag)
            return push_nil1(vm);

        vm->pushString(tag->value.c_str());
        return 1;
    }

    static int entity_remove_tag(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.removeTag() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.removeTag()", &runtime);
        if (!entity)
            return push_nil1(vm);

        vm->pushBool(runtime->registry.remove<TagComponent>(entity->entity) > 0u);
        return 1;
    }

    static int entity_has_transform2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.hasTransform2D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.hasTransform2D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        vm->pushBool(runtime->registry.all_of<Transform2DComponent>(entity->entity));
        return 1;
    }

    static int entity_set_transform2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 2 || argCount > 5)
        {
            Error("ECSEntity.setTransform2D(x, y [, rotation [, sx [, sy]]])");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setTransform2D()", &runtime);
        if (!entity)
            return 0;

        double x = 0.0;
        double y = 0.0;
        double rotation = 0.0;
        double sx = 1.0;
        double sy = 1.0;

        if (!read_number_arg(args[0], &x, "ECSEntity.setTransform2D()", 1) ||
            !read_number_arg(args[1], &y, "ECSEntity.setTransform2D()", 2))
        {
            return 0;
        }

        Transform2DComponent t = read_or_default_transform(runtime, entity->entity);
        t.x = (float)x;
        t.y = (float)y;

        if (argCount >= 3)
        {
            if (!read_number_arg(args[2], &rotation, "ECSEntity.setTransform2D()", 3))
                return 0;
            t.rotation = (float)rotation;
        }

        if (argCount >= 4)
        {
            if (!read_number_arg(args[3], &sx, "ECSEntity.setTransform2D()", 4))
                return 0;
            t.sx = (float)sx;
        }

        if (argCount >= 5)
        {
            if (!read_number_arg(args[4], &sy, "ECSEntity.setTransform2D()", 5))
                return 0;
            t.sy = (float)sy;
        }

        runtime->registry.emplace_or_replace<Transform2DComponent>(entity->entity, t);
        return 0;
    }

    static int entity_get_transform2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getTransform2D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getTransform2D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Transform2DComponent *t = runtime->registry.try_get<Transform2DComponent>(entity->entity);
        if (!t)
            return push_nil1(vm);

        vm->pushDouble((double)t->x);
        vm->pushDouble((double)t->y);
        vm->pushDouble((double)t->rotation);
        vm->pushDouble((double)t->sx);
        vm->pushDouble((double)t->sy);
        return 5;
    }

    static int entity_set_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("ECSEntity.setPosition(x, y)");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setPosition()", &runtime);
        if (!entity)
            return 0;

        double x = 0.0;
        double y = 0.0;
        if (!read_number_arg(args[0], &x, "ECSEntity.setPosition()", 1) ||
            !read_number_arg(args[1], &y, "ECSEntity.setPosition()", 2))
        {
            return 0;
        }

        Transform2DComponent t = read_or_default_transform(runtime, entity->entity);
        t.x = (float)x;
        t.y = (float)y;
        runtime->registry.emplace_or_replace<Transform2DComponent>(entity->entity, t);
        return 0;
    }

    static int entity_get_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getPosition() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getPosition()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Transform2DComponent *t = runtime->registry.try_get<Transform2DComponent>(entity->entity);
        if (!t)
            return push_nil1(vm);

        vm->pushDouble((double)t->x);
        vm->pushDouble((double)t->y);
        return 2;
    }

    static int entity_set_rotation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSEntity.setRotation(rotation)");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setRotation()", &runtime);
        if (!entity)
            return 0;

        double rotation = 0.0;
        if (!read_number_arg(args[0], &rotation, "ECSEntity.setRotation()", 1))
            return 0;

        Transform2DComponent t = read_or_default_transform(runtime, entity->entity);
        t.rotation = (float)rotation;
        runtime->registry.emplace_or_replace<Transform2DComponent>(entity->entity, t);
        return 0;
    }

    static int entity_get_rotation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getRotation() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getRotation()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Transform2DComponent *t = runtime->registry.try_get<Transform2DComponent>(entity->entity);
        if (!t)
            return push_nil1(vm);

        vm->pushDouble((double)t->rotation);
        return 1;
    }

    static int entity_set_scale(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("ECSEntity.setScale(sx, sy)");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setScale()", &runtime);
        if (!entity)
            return 0;

        double sx = 1.0;
        double sy = 1.0;
        if (!read_number_arg(args[0], &sx, "ECSEntity.setScale()", 1) ||
            !read_number_arg(args[1], &sy, "ECSEntity.setScale()", 2))
        {
            return 0;
        }

        Transform2DComponent t = read_or_default_transform(runtime, entity->entity);
        t.sx = (float)sx;
        t.sy = (float)sy;
        runtime->registry.emplace_or_replace<Transform2DComponent>(entity->entity, t);
        return 0;
    }

    static int entity_get_scale(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getScale() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getScale()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Transform2DComponent *t = runtime->registry.try_get<Transform2DComponent>(entity->entity);
        if (!t)
            return push_nil1(vm);

        vm->pushDouble((double)t->sx);
        vm->pushDouble((double)t->sy);
        return 2;
    }

    static int entity_remove_transform2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.removeTransform2D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.removeTransform2D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        vm->pushBool(runtime->registry.remove<Transform2DComponent>(entity->entity) > 0u);
        return 1;
    }

    static int entity_has_transform3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.hasTransform3D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.hasTransform3D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        vm->pushBool(runtime->registry.all_of<Transform3DComponent>(entity->entity));
        return 1;
    }

    static int entity_set_transform3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 3 || argCount > 9)
        {
            Error("ECSEntity.setTransform3D(x, y, z [, rx [, ry [, rz [, sx [, sy [, sz]]]]]])");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setTransform3D()", &runtime);
        if (!entity)
            return 0;

        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        double rx = 0.0;
        double ry = 0.0;
        double rz = 0.0;
        double sx = 1.0;
        double sy = 1.0;
        double sz = 1.0;

        if (!read_number_arg(args[0], &x, "ECSEntity.setTransform3D()", 1) ||
            !read_number_arg(args[1], &y, "ECSEntity.setTransform3D()", 2) ||
            !read_number_arg(args[2], &z, "ECSEntity.setTransform3D()", 3))
        {
            return 0;
        }

        Transform3DComponent t = read_or_default_transform3d(runtime, entity->entity);
        t.x = (float)x;
        t.y = (float)y;
        t.z = (float)z;

        if (argCount >= 4)
        {
            if (!read_number_arg(args[3], &rx, "ECSEntity.setTransform3D()", 4))
                return 0;
            t.rx = (float)rx;
        }

        if (argCount >= 5)
        {
            if (!read_number_arg(args[4], &ry, "ECSEntity.setTransform3D()", 5))
                return 0;
            t.ry = (float)ry;
        }

        if (argCount >= 6)
        {
            if (!read_number_arg(args[5], &rz, "ECSEntity.setTransform3D()", 6))
                return 0;
            t.rz = (float)rz;
        }

        if (argCount >= 7)
        {
            if (!read_number_arg(args[6], &sx, "ECSEntity.setTransform3D()", 7))
                return 0;
            t.sx = (float)sx;
        }

        if (argCount >= 8)
        {
            if (!read_number_arg(args[7], &sy, "ECSEntity.setTransform3D()", 8))
                return 0;
            t.sy = (float)sy;
        }

        if (argCount >= 9)
        {
            if (!read_number_arg(args[8], &sz, "ECSEntity.setTransform3D()", 9))
                return 0;
            t.sz = (float)sz;
        }

        runtime->registry.emplace_or_replace<Transform3DComponent>(entity->entity, t);
        return 0;
    }

    static int entity_get_transform3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getTransform3D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getTransform3D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Transform3DComponent *t = runtime->registry.try_get<Transform3DComponent>(entity->entity);
        if (!t)
            return push_nil1(vm);

        vm->pushDouble((double)t->x);
        vm->pushDouble((double)t->y);
        vm->pushDouble((double)t->z);
        vm->pushDouble((double)t->rx);
        vm->pushDouble((double)t->ry);
        vm->pushDouble((double)t->rz);
        vm->pushDouble((double)t->sx);
        vm->pushDouble((double)t->sy);
        vm->pushDouble((double)t->sz);
        return 9;
    }

    static int entity_set_position3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("ECSEntity.setPosition3D(x, y, z)");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setPosition3D()", &runtime);
        if (!entity)
            return 0;

        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        if (!read_number_arg(args[0], &x, "ECSEntity.setPosition3D()", 1) ||
            !read_number_arg(args[1], &y, "ECSEntity.setPosition3D()", 2) ||
            !read_number_arg(args[2], &z, "ECSEntity.setPosition3D()", 3))
        {
            return 0;
        }

        Transform3DComponent t = read_or_default_transform3d(runtime, entity->entity);
        t.x = (float)x;
        t.y = (float)y;
        t.z = (float)z;
        runtime->registry.emplace_or_replace<Transform3DComponent>(entity->entity, t);
        return 0;
    }

    static int entity_get_position3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getPosition3D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getPosition3D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Transform3DComponent *t = runtime->registry.try_get<Transform3DComponent>(entity->entity);
        if (!t)
            return push_nil1(vm);

        vm->pushDouble((double)t->x);
        vm->pushDouble((double)t->y);
        vm->pushDouble((double)t->z);
        return 3;
    }

    static int entity_set_rotation3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("ECSEntity.setRotation3D(rx, ry, rz)");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setRotation3D()", &runtime);
        if (!entity)
            return 0;

        double rx = 0.0;
        double ry = 0.0;
        double rz = 0.0;
        if (!read_number_arg(args[0], &rx, "ECSEntity.setRotation3D()", 1) ||
            !read_number_arg(args[1], &ry, "ECSEntity.setRotation3D()", 2) ||
            !read_number_arg(args[2], &rz, "ECSEntity.setRotation3D()", 3))
        {
            return 0;
        }

        Transform3DComponent t = read_or_default_transform3d(runtime, entity->entity);
        t.rx = (float)rx;
        t.ry = (float)ry;
        t.rz = (float)rz;
        runtime->registry.emplace_or_replace<Transform3DComponent>(entity->entity, t);
        return 0;
    }

    static int entity_get_rotation3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getRotation3D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getRotation3D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Transform3DComponent *t = runtime->registry.try_get<Transform3DComponent>(entity->entity);
        if (!t)
            return push_nil1(vm);

        vm->pushDouble((double)t->rx);
        vm->pushDouble((double)t->ry);
        vm->pushDouble((double)t->rz);
        return 3;
    }

    static int entity_set_scale3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("ECSEntity.setScale3D(sx, sy, sz)");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setScale3D()", &runtime);
        if (!entity)
            return 0;

        double sx = 1.0;
        double sy = 1.0;
        double sz = 1.0;
        if (!read_number_arg(args[0], &sx, "ECSEntity.setScale3D()", 1) ||
            !read_number_arg(args[1], &sy, "ECSEntity.setScale3D()", 2) ||
            !read_number_arg(args[2], &sz, "ECSEntity.setScale3D()", 3))
        {
            return 0;
        }

        Transform3DComponent t = read_or_default_transform3d(runtime, entity->entity);
        t.sx = (float)sx;
        t.sy = (float)sy;
        t.sz = (float)sz;
        runtime->registry.emplace_or_replace<Transform3DComponent>(entity->entity, t);
        return 0;
    }

    static int entity_get_scale3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getScale3D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getScale3D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Transform3DComponent *t = runtime->registry.try_get<Transform3DComponent>(entity->entity);
        if (!t)
            return push_nil1(vm);

        vm->pushDouble((double)t->sx);
        vm->pushDouble((double)t->sy);
        vm->pushDouble((double)t->sz);
        return 3;
    }

    static int entity_remove_transform3d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.removeTransform3D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.removeTransform3D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        vm->pushBool(runtime->registry.remove<Transform3DComponent>(entity->entity) > 0u);
        return 1;
    }

    static int entity_has_velocity2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.hasVelocity2D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.hasVelocity2D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        vm->pushBool(runtime->registry.all_of<Velocity2DComponent>(entity->entity));
        return 1;
    }

    static int entity_set_velocity2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("ECSEntity.setVelocity2D(vx, vy)");
            return 0;
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.setVelocity2D()", &runtime);
        if (!entity)
            return 0;

        double vx = 0.0;
        double vy = 0.0;
        if (!read_number_arg(args[0], &vx, "ECSEntity.setVelocity2D()", 1) ||
            !read_number_arg(args[1], &vy, "ECSEntity.setVelocity2D()", 2))
        {
            return 0;
        }

        Velocity2DComponent v = read_or_default_velocity(runtime, entity->entity);
        v.vx = (float)vx;
        v.vy = (float)vy;
        runtime->registry.emplace_or_replace<Velocity2DComponent>(entity->entity, v);
        return 0;
    }

    static int entity_get_velocity2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.getVelocity2D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getVelocity2D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        Velocity2DComponent *v = runtime->registry.try_get<Velocity2DComponent>(entity->entity);
        if (!v)
            return push_nil1(vm);

        vm->pushDouble((double)v->vx);
        vm->pushDouble((double)v->vy);
        return 2;
    }

    static int entity_remove_velocity2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSEntity.removeVelocity2D() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.removeVelocity2D()", &runtime);
        if (!entity)
            return push_nil1(vm);

        vm->pushBool(runtime->registry.remove<Velocity2DComponent>(entity->entity) > 0u);
        return 1;
    }

    static int entity_has_component(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSEntity.hasComponent(name)");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.hasComponent()", &runtime);
        if (!entity)
            return push_nil1(vm);

        const char *name = nullptr;
        if (!read_string_arg(args[0], &name, "ECSEntity.hasComponent()", 1))
            return push_nil1(vm);

        bool has = false;
        switch (parse_component_kind(name))
        {
        case ComponentKind::Tag:
            has = runtime->registry.all_of<TagComponent>(entity->entity);
            break;
        case ComponentKind::Transform2D:
            has = runtime->registry.all_of<Transform2DComponent>(entity->entity);
            break;
        case ComponentKind::Velocity2D:
            has = runtime->registry.all_of<Velocity2DComponent>(entity->entity);
            break;
        case ComponentKind::Transform3D:
            has = runtime->registry.all_of<Transform3DComponent>(entity->entity);
            break;
        default:
            Error("ECSEntity.hasComponent() unknown component '%s'", name ? name : "<null>");
            return push_nil1(vm);
        }

        vm->pushBool(has);
        return 1;
    }

    static int entity_add_component(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1)
        {
            Error("ECSEntity.addComponent(name [, ...])");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.addComponent()", &runtime);
        if (!entity)
            return push_nil1(vm);

        const char *name = nullptr;
        if (!read_string_arg(args[0], &name, "ECSEntity.addComponent()", 1))
            return push_nil1(vm);

        switch (parse_component_kind(name))
        {
        case ComponentKind::Tag:
        {
            const char *tag = "";
            if (argCount >= 2 && !read_string_arg(args[1], &tag, "ECSEntity.addComponent()", 2))
                return push_nil1(vm);
            runtime->registry.emplace_or_replace<TagComponent>(entity->entity, TagComponent{tag ? tag : ""});
            vm->pushBool(true);
            return 1;
        }
        case ComponentKind::Transform2D:
        {
            double x = 0.0, y = 0.0, rotation = 0.0, sx = 1.0, sy = 1.0;
            if (argCount >= 2 && !read_number_arg(args[1], &x, "ECSEntity.addComponent()", 2)) return push_nil1(vm);
            if (argCount >= 3 && !read_number_arg(args[2], &y, "ECSEntity.addComponent()", 3)) return push_nil1(vm);
            if (argCount >= 4 && !read_number_arg(args[3], &rotation, "ECSEntity.addComponent()", 4)) return push_nil1(vm);
            if (argCount >= 5 && !read_number_arg(args[4], &sx, "ECSEntity.addComponent()", 5)) return push_nil1(vm);
            if (argCount >= 6 && !read_number_arg(args[5], &sy, "ECSEntity.addComponent()", 6)) return push_nil1(vm);
            if (argCount > 6)
            {
                Error("ECSEntity.addComponent(Transform2D) accepts up to 5 values");
                return push_nil1(vm);
            }

            runtime->registry.emplace_or_replace<Transform2DComponent>(
                entity->entity,
                Transform2DComponent{(float)x, (float)y, (float)rotation, (float)sx, (float)sy});
            vm->pushBool(true);
            return 1;
        }
        case ComponentKind::Velocity2D:
        {
            double vx = 0.0, vy = 0.0;
            if (argCount >= 2 && !read_number_arg(args[1], &vx, "ECSEntity.addComponent()", 2)) return push_nil1(vm);
            if (argCount >= 3 && !read_number_arg(args[2], &vy, "ECSEntity.addComponent()", 3)) return push_nil1(vm);
            if (argCount > 3)
            {
                Error("ECSEntity.addComponent(Velocity2D) accepts up to 2 values");
                return push_nil1(vm);
            }

            runtime->registry.emplace_or_replace<Velocity2DComponent>(
                entity->entity,
                Velocity2DComponent{(float)vx, (float)vy});
            vm->pushBool(true);
            return 1;
        }
        case ComponentKind::Transform3D:
        {
            double x = 0.0, y = 0.0, z = 0.0;
            double rx = 0.0, ry = 0.0, rz = 0.0;
            double sx = 1.0, sy = 1.0, sz = 1.0;
            if (argCount >= 2 && !read_number_arg(args[1], &x, "ECSEntity.addComponent()", 2)) return push_nil1(vm);
            if (argCount >= 3 && !read_number_arg(args[2], &y, "ECSEntity.addComponent()", 3)) return push_nil1(vm);
            if (argCount >= 4 && !read_number_arg(args[3], &z, "ECSEntity.addComponent()", 4)) return push_nil1(vm);
            if (argCount >= 5 && !read_number_arg(args[4], &rx, "ECSEntity.addComponent()", 5)) return push_nil1(vm);
            if (argCount >= 6 && !read_number_arg(args[5], &ry, "ECSEntity.addComponent()", 6)) return push_nil1(vm);
            if (argCount >= 7 && !read_number_arg(args[6], &rz, "ECSEntity.addComponent()", 7)) return push_nil1(vm);
            if (argCount >= 8 && !read_number_arg(args[7], &sx, "ECSEntity.addComponent()", 8)) return push_nil1(vm);
            if (argCount >= 9 && !read_number_arg(args[8], &sy, "ECSEntity.addComponent()", 9)) return push_nil1(vm);
            if (argCount >= 10 && !read_number_arg(args[9], &sz, "ECSEntity.addComponent()", 10)) return push_nil1(vm);
            if (argCount > 10)
            {
                Error("ECSEntity.addComponent(Transform3D) accepts up to 9 values");
                return push_nil1(vm);
            }

            runtime->registry.emplace_or_replace<Transform3DComponent>(
                entity->entity,
                Transform3DComponent{(float)x, (float)y, (float)z,
                                     (float)rx, (float)ry, (float)rz,
                                     (float)sx, (float)sy, (float)sz});
            vm->pushBool(true);
            return 1;
        }
        default:
            Error("ECSEntity.addComponent() unknown component '%s'", name ? name : "<null>");
            return push_nil1(vm);
        }
    }

    static int entity_set_component(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        return entity_add_component(vm, instance, argCount, args);
    }

    static int entity_get_component(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSEntity.getComponent(name)");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.getComponent()", &runtime);
        if (!entity)
            return push_nil1(vm);

        const char *name = nullptr;
        if (!read_string_arg(args[0], &name, "ECSEntity.getComponent()", 1))
            return push_nil1(vm);

        switch (parse_component_kind(name))
        {
        case ComponentKind::Tag:
        {
            TagComponent *c = runtime->registry.try_get<TagComponent>(entity->entity);
            if (!c) return push_nil1(vm);
            vm->pushString(c->value.c_str());
            return 1;
        }
        case ComponentKind::Transform2D:
        {
            Transform2DComponent *c = runtime->registry.try_get<Transform2DComponent>(entity->entity);
            if (!c) return push_nil1(vm);
            vm->pushDouble((double)c->x);
            vm->pushDouble((double)c->y);
            vm->pushDouble((double)c->rotation);
            vm->pushDouble((double)c->sx);
            vm->pushDouble((double)c->sy);
            return 5;
        }
        case ComponentKind::Velocity2D:
        {
            Velocity2DComponent *c = runtime->registry.try_get<Velocity2DComponent>(entity->entity);
            if (!c) return push_nil1(vm);
            vm->pushDouble((double)c->vx);
            vm->pushDouble((double)c->vy);
            return 2;
        }
        case ComponentKind::Transform3D:
        {
            Transform3DComponent *c = runtime->registry.try_get<Transform3DComponent>(entity->entity);
            if (!c) return push_nil1(vm);
            vm->pushDouble((double)c->x);
            vm->pushDouble((double)c->y);
            vm->pushDouble((double)c->z);
            vm->pushDouble((double)c->rx);
            vm->pushDouble((double)c->ry);
            vm->pushDouble((double)c->rz);
            vm->pushDouble((double)c->sx);
            vm->pushDouble((double)c->sy);
            vm->pushDouble((double)c->sz);
            return 9;
        }
        default:
            Error("ECSEntity.getComponent() unknown component '%s'", name ? name : "<null>");
            return push_nil1(vm);
        }
    }

    static int entity_remove_component(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSEntity.removeComponent(name)");
            return push_nil1(vm);
        }

        WorldRuntime *runtime = nullptr;
        EntityHandle *entity = require_entity(instance, "ECSEntity.removeComponent()", &runtime);
        if (!entity)
            return push_nil1(vm);

        const char *name = nullptr;
        if (!read_string_arg(args[0], &name, "ECSEntity.removeComponent()", 1))
            return push_nil1(vm);

        bool removed = false;
        switch (parse_component_kind(name))
        {
        case ComponentKind::Tag:
            removed = runtime->registry.remove<TagComponent>(entity->entity) > 0u;
            break;
        case ComponentKind::Transform2D:
            removed = runtime->registry.remove<Transform2DComponent>(entity->entity) > 0u;
            break;
        case ComponentKind::Velocity2D:
            removed = runtime->registry.remove<Velocity2DComponent>(entity->entity) > 0u;
            break;
        case ComponentKind::Transform3D:
            removed = runtime->registry.remove<Transform3DComponent>(entity->entity) > 0u;
            break;
        default:
            Error("ECSEntity.removeComponent() unknown component '%s'", name ? name : "<null>");
            return push_nil1(vm);
        }

        vm->pushBool(removed);
        return 1;
    }

    void register_entity(Interpreter &vm)
    {
        g_entityClass = vm.registerNativeClass("ECSEntity", entt_entity_ctor, entt_entity_dtor, 0, false);

        vm.addNativeMethod(g_entityClass, "destroy", entity_destroy);
        vm.addNativeMethod(g_entityClass, "isValid", entity_is_valid);
        vm.addNativeMethod(g_entityClass, "getId", entity_get_id);

        vm.addNativeMethod(g_entityClass, "hasTag", entity_has_tag);
        vm.addNativeMethod(g_entityClass, "setTag", entity_set_tag);
        vm.addNativeMethod(g_entityClass, "getTag", entity_get_tag);
        vm.addNativeMethod(g_entityClass, "removeTag", entity_remove_tag);

        vm.addNativeMethod(g_entityClass, "hasTransform2D", entity_has_transform2d);
        vm.addNativeMethod(g_entityClass, "setTransform2D", entity_set_transform2d);
        vm.addNativeMethod(g_entityClass, "getTransform2D", entity_get_transform2d);
        vm.addNativeMethod(g_entityClass, "setPosition", entity_set_position);
        vm.addNativeMethod(g_entityClass, "getPosition", entity_get_position);
        vm.addNativeMethod(g_entityClass, "setRotation", entity_set_rotation);
        vm.addNativeMethod(g_entityClass, "getRotation", entity_get_rotation);
        vm.addNativeMethod(g_entityClass, "setScale", entity_set_scale);
        vm.addNativeMethod(g_entityClass, "getScale", entity_get_scale);
        vm.addNativeMethod(g_entityClass, "removeTransform2D", entity_remove_transform2d);
        vm.addNativeMethod(g_entityClass, "hasTransform3D", entity_has_transform3d);
        vm.addNativeMethod(g_entityClass, "setTransform3D", entity_set_transform3d);
        vm.addNativeMethod(g_entityClass, "getTransform3D", entity_get_transform3d);
        vm.addNativeMethod(g_entityClass, "setPosition3D", entity_set_position3d);
        vm.addNativeMethod(g_entityClass, "getPosition3D", entity_get_position3d);
        vm.addNativeMethod(g_entityClass, "setRotation3D", entity_set_rotation3d);
        vm.addNativeMethod(g_entityClass, "getRotation3D", entity_get_rotation3d);
        vm.addNativeMethod(g_entityClass, "setScale3D", entity_set_scale3d);
        vm.addNativeMethod(g_entityClass, "getScale3D", entity_get_scale3d);
        vm.addNativeMethod(g_entityClass, "removeTransform3D", entity_remove_transform3d);

        vm.addNativeMethod(g_entityClass, "hasVelocity2D", entity_has_velocity2d);
        vm.addNativeMethod(g_entityClass, "setVelocity2D", entity_set_velocity2d);
        vm.addNativeMethod(g_entityClass, "getVelocity2D", entity_get_velocity2d);
        vm.addNativeMethod(g_entityClass, "removeVelocity2D", entity_remove_velocity2d);

        vm.addNativeMethod(g_entityClass, "hasComponent", entity_has_component);
        vm.addNativeMethod(g_entityClass, "addComponent", entity_add_component);
        vm.addNativeMethod(g_entityClass, "setComponent", entity_set_component);
        vm.addNativeMethod(g_entityClass, "getComponent", entity_get_component);
        vm.addNativeMethod(g_entityClass, "removeComponent", entity_remove_component);
    }
}
