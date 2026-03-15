#include "entt_core.hpp"

#include <vector>

namespace EnTTBindings
{
    // ctor/dtor lives in entt_core.cpp
    void *entt_world_ctor(Interpreter *vm, int argCount, Value *args);
    void entt_world_dtor(Interpreter *vm, void *instance);

    static int world_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.destroy() expects 0 arguments");
            return 0;
        }

        WorldHandle *world = (WorldHandle *)instance;
        if (world)
            destroy_world_runtime(world);

        return 0;
    }

    static int world_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldHandle *world = (WorldHandle *)instance;
        vm->pushBool(world && world->valid && find_world_runtime(world->worldId));
        return 1;
    }

    static int world_clear(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.clear() expects 0 arguments");
            return 0;
        }

        WorldHandle *world = require_world(instance, "ECSWorld.clear()");
        if (!world)
            return 0;

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return 0;

        runtime->registry.clear();
        return 0;
    }

    static int world_get_entity_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.getEntityCount() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getEntityCount()");
        if (!world)
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto &storage = runtime->registry.storage<entt::entity>();
        for (auto [entity] : storage.each())
        {
            (void)entity;
            ++count;
        }

        vm->pushInt(count);
        return 1;
    }

    static int world_create_entity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 0 && argCount != 1)
        {
            Error("ECSWorld.createEntity([tag])");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.createEntity()");
        if (!world)
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        const char *tag = nullptr;
        if (argCount == 1)
        {
            if (!read_string_arg(args[0], &tag, "ECSWorld.createEntity()", 1))
                return push_nil1(vm);
        }

        entt::entity entity = runtime->registry.create();
        if (tag)
            runtime->registry.emplace_or_replace<TagComponent>(entity, TagComponent{tag});

        return push_entity_handle(vm, world->worldId, entity) ? 1 : push_nil1(vm);
    }

    static int world_destroy_entity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSWorld.destroyEntity(entity)");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.destroyEntity()");
        if (!world)
            return push_nil1(vm);

        EntityHandle *entity = nullptr;
        if (!read_entity_arg(args[0], &entity, "ECSWorld.destroyEntity()", 1))
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime || !entity || entity->worldId != world->worldId || !entity->valid || entity->entity == entt::null)
        {
            vm->pushBool(false);
            return 1;
        }

        if (!runtime->registry.valid(entity->entity))
        {
            vm->pushBool(false);
            return 1;
        }

        runtime->registry.destroy(entity->entity);
        entity->entity = entt::null;
        entity->valid = false;
        vm->pushBool(true);
        return 1;
    }

    static int world_is_alive(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSWorld.isAlive(entity)");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.isAlive()");
        if (!world)
            return push_nil1(vm);

        EntityHandle *entity = nullptr;
        if (!read_entity_arg(args[0], &entity, "ECSWorld.isAlive()", 1))
            return push_nil1(vm);

        if (!entity || entity->worldId != world->worldId)
        {
            vm->pushBool(false);
            return 1;
        }

        vm->pushBool(is_entity_alive(entity));
        return 1;
    }

    static int world_get_entity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSWorld.getEntity(id)");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getEntity()");
        if (!world)
            return push_nil1(vm);

        double entityId = 0.0;
        if (!read_number_arg(args[0], &entityId, "ECSWorld.getEntity()", 1))
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        entt::entity entity = id_to_entity((int)entityId);
        if (entity == entt::null || !runtime->registry.valid(entity))
            return push_nil1(vm);

        return push_entity_handle(vm, world->worldId, entity) ? 1 : push_nil1(vm);
    }

    static int world_get_entity_ids(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.getEntityIds() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getEntityIds()");
        if (!world)
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto &storage = runtime->registry.storage<entt::entity>();
        for (auto [entity] : storage.each())
            (void)entity, ++count;

        vm->pushInt(count);
        for (auto [entity] : storage.each())
            vm->pushInt(entity_to_id(entity));
        return count + 1;
    }

    static int world_find_by_tag(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSWorld.findByTag(tag)");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.findByTag()");
        if (!world)
            return push_nil1(vm);

        const char *wanted = nullptr;
        if (!read_string_arg(args[0], &wanted, "ECSWorld.findByTag()", 1))
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        auto view = runtime->registry.view<TagComponent>();
        for (auto entity : view)
        {
            const TagComponent &tag = view.get<TagComponent>(entity);
            if (tag.value == (wanted ? wanted : ""))
                return push_entity_handle(vm, world->worldId, entity) ? 1 : push_nil1(vm);
        }

        return push_nil1(vm);
    }

    static int world_get_tagged_ids(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSWorld.getTaggedIds(tag)");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getTaggedIds()");
        if (!world)
            return push_nil1(vm);

        const char *wanted = nullptr;
        if (!read_string_arg(args[0], &wanted, "ECSWorld.getTaggedIds()", 1))
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto view = runtime->registry.view<TagComponent>();
        for (auto entity : view)
        {
            const TagComponent &tag = view.get<TagComponent>(entity);
            if (tag.value == (wanted ? wanted : ""))
                ++count;
        }

        vm->pushInt(count);
        for (auto entity : view)
        {
            const TagComponent &tag = view.get<TagComponent>(entity);
            if (tag.value == (wanted ? wanted : ""))
                vm->pushInt(entity_to_id(entity));
        }
        return count + 1;
    }

    static int world_get_transform2d_ids(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.getTransform2DIds() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getTransform2DIds()");
        if (!world)
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto view = runtime->registry.view<Transform2DComponent>();
        for (auto entity : view)
            (void)entity, ++count;

        vm->pushInt(count);
        for (auto entity : view)
            vm->pushInt(entity_to_id(entity));
        return count + 1;
    }

    static int world_get_velocity2d_ids(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.getVelocity2DIds() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getVelocity2DIds()");
        if (!world)
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto view = runtime->registry.view<Velocity2DComponent>();
        for (auto entity : view)
            (void)entity, ++count;

        vm->pushInt(count);
        for (auto entity : view)
            vm->pushInt(entity_to_id(entity));
        return count + 1;
    }

    static int world_get_transform2d_data(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.getTransform2DData() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getTransform2DData()");
        if (!world)
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto view = runtime->registry.view<Transform2DComponent>();
        for (auto entity : view)
            (void)entity, ++count;

        vm->pushInt(count);
        for (auto entity : view)
        {
            const Transform2DComponent &t = view.get<Transform2DComponent>(entity);
            vm->pushInt(entity_to_id(entity));
            vm->pushDouble((double)t.x);
            vm->pushDouble((double)t.y);
            vm->pushDouble((double)t.rotation);
            vm->pushDouble((double)t.sx);
            vm->pushDouble((double)t.sy);
        }
        return count * 6 + 1;
    }

    static int world_get_transform3d_ids(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.getTransform3DIds() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getTransform3DIds()");
        if (!world)
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto view = runtime->registry.view<Transform3DComponent>();
        for (auto entity : view)
            (void)entity, ++count;

        vm->pushInt(count);
        for (auto entity : view)
            vm->pushInt(entity_to_id(entity));
        return count + 1;
    }

    static int world_get_transform3d_data(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld.getTransform3DData() expects 0 arguments");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.getTransform3DData()");
        if (!world)
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto view = runtime->registry.view<Transform3DComponent>();
        for (auto entity : view)
            (void)entity, ++count;

        vm->pushInt(count);
        for (auto entity : view)
        {
            const Transform3DComponent &t = view.get<Transform3DComponent>(entity);
            vm->pushInt(entity_to_id(entity));
            vm->pushDouble((double)t.x);
            vm->pushDouble((double)t.y);
            vm->pushDouble((double)t.z);
            vm->pushDouble((double)t.rx);
            vm->pushDouble((double)t.ry);
            vm->pushDouble((double)t.rz);
            vm->pushDouble((double)t.sx);
            vm->pushDouble((double)t.sy);
            vm->pushDouble((double)t.sz);
        }
        return count * 10 + 1;
    }

    static int world_step_velocity2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSWorld.stepVelocity2D(dt)");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.stepVelocity2D()");
        if (!world)
            return push_nil1(vm);

        double dt = 0.0;
        if (!read_number_arg(args[0], &dt, "ECSWorld.stepVelocity2D()", 1))
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto view = runtime->registry.view<Transform2DComponent, Velocity2DComponent>();
        for (auto entity : view)
        {
            Transform2DComponent &t = view.get<Transform2DComponent>(entity);
            const Velocity2DComponent &v = view.get<Velocity2DComponent>(entity);
            t.x += v.vx * (float)dt;
            t.y += v.vy * (float)dt;
            ++count;
        }

        vm->pushInt(count);
        return 1;
    }

    static int world_rotate_all_transform2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("ECSWorld.rotateAllTransform2D(deltaDegrees)");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.rotateAllTransform2D()");
        if (!world)
            return push_nil1(vm);

        double delta = 0.0;
        if (!read_number_arg(args[0], &delta, "ECSWorld.rotateAllTransform2D()", 1))
            return push_nil1(vm);

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int count = 0;
        auto view = runtime->registry.view<Transform2DComponent>();
        for (auto entity : view)
        {
            Transform2DComponent &t = view.get<Transform2DComponent>(entity);
            t.rotation += (float)delta;
            ++count;
        }

        vm->pushInt(count);
        return 1;
    }

    static int world_bounce_velocity2d(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 4 && argCount != 5)
        {
            Error("ECSWorld.bounceVelocity2D(minX, minY, maxX, maxY [, bounceFactor])");
            return push_nil1(vm);
        }

        WorldHandle *world = require_world(instance, "ECSWorld.bounceVelocity2D()");
        if (!world)
            return push_nil1(vm);

        double minX = 0.0;
        double minY = 0.0;
        double maxX = 0.0;
        double maxY = 0.0;
        double bounceFactor = 1.0;

        if (!read_number_arg(args[0], &minX, "ECSWorld.bounceVelocity2D()", 1) ||
            !read_number_arg(args[1], &minY, "ECSWorld.bounceVelocity2D()", 2) ||
            !read_number_arg(args[2], &maxX, "ECSWorld.bounceVelocity2D()", 3) ||
            !read_number_arg(args[3], &maxY, "ECSWorld.bounceVelocity2D()", 4))
        {
            return push_nil1(vm);
        }

        if (argCount == 5 && !read_number_arg(args[4], &bounceFactor, "ECSWorld.bounceVelocity2D()", 5))
            return push_nil1(vm);

        if (bounceFactor < 0.0)
            bounceFactor = 0.0;

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
            return push_nil1(vm);

        int hitCount = 0;
        auto view = runtime->registry.view<Transform2DComponent, Velocity2DComponent>();
        for (auto entity : view)
        {
            Transform2DComponent &t = view.get<Transform2DComponent>(entity);
            Velocity2DComponent &v = view.get<Velocity2DComponent>(entity);

            if (t.x < (float)minX)
            {
                t.x = (float)minX;
                if (v.vx < 0.0f)
                {
                    v.vx = -v.vx * (float)bounceFactor;
                    ++hitCount;
                }
            }
            else if (t.x > (float)maxX)
            {
                t.x = (float)maxX;
                if (v.vx > 0.0f)
                {
                    v.vx = -v.vx * (float)bounceFactor;
                    ++hitCount;
                }
            }

            if (t.y < (float)minY)
            {
                t.y = (float)minY;
                if (v.vy < 0.0f)
                {
                    v.vy = -v.vy * (float)bounceFactor;
                    ++hitCount;
                }
            }
            else if (t.y > (float)maxY)
            {
                t.y = (float)maxY;
                if (v.vy > 0.0f)
                {
                    v.vy = -v.vy * (float)bounceFactor;
                    ++hitCount;
                }
            }
        }

        vm->pushInt(hitCount);
        return 1;
    }

    void register_world(Interpreter &vm)
    {
        g_worldClass = vm.registerNativeClass("ECSWorld", entt_world_ctor, entt_world_dtor, 0, false);

        vm.addNativeMethod(g_worldClass, "destroy", world_destroy);
        vm.addNativeMethod(g_worldClass, "isValid", world_is_valid);
        vm.addNativeMethod(g_worldClass, "clear", world_clear);
        vm.addNativeMethod(g_worldClass, "getEntityCount", world_get_entity_count);
        vm.addNativeMethod(g_worldClass, "createEntity", world_create_entity);
        vm.addNativeMethod(g_worldClass, "destroyEntity", world_destroy_entity);
        vm.addNativeMethod(g_worldClass, "isAlive", world_is_alive);
        vm.addNativeMethod(g_worldClass, "getEntity", world_get_entity);
        vm.addNativeMethod(g_worldClass, "getEntityIds", world_get_entity_ids);
        vm.addNativeMethod(g_worldClass, "findByTag", world_find_by_tag);
        vm.addNativeMethod(g_worldClass, "getTaggedIds", world_get_tagged_ids);
        vm.addNativeMethod(g_worldClass, "getTransform2DIds", world_get_transform2d_ids);
        vm.addNativeMethod(g_worldClass, "getVelocity2DIds", world_get_velocity2d_ids);
        vm.addNativeMethod(g_worldClass, "getTransform2DData", world_get_transform2d_data);
        vm.addNativeMethod(g_worldClass, "getTransform3DIds", world_get_transform3d_ids);
        vm.addNativeMethod(g_worldClass, "getTransform3DData", world_get_transform3d_data);
        vm.addNativeMethod(g_worldClass, "stepVelocity2D", world_step_velocity2d);
        vm.addNativeMethod(g_worldClass, "rotateAllTransform2D", world_rotate_all_transform2d);
        vm.addNativeMethod(g_worldClass, "bounceVelocity2D", world_bounce_velocity2d);
    }
}
