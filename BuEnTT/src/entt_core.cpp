#include "entt_core.hpp"

#include <unordered_map>

namespace EnTTBindings
{
    NativeClassDef *g_worldClass = nullptr;
    NativeClassDef *g_entityClass = nullptr;

    static std::unordered_map<std::uint64_t, WorldRuntime *> g_worlds;
    static std::uint64_t g_nextWorldId = 1;

    int push_nil1(Interpreter *vm)
    {
        vm->pushNil();
        return 1;
    }

    bool read_number_arg(const Value &value, double *out, const char *fn, int argIndex)
    {
        if (!out || !value.isNumber())
        {
            Error("%s arg %d expects number", fn, argIndex);
            return false;
        }

        *out = value.asNumber();
        return true;
    }

    bool read_string_arg(const Value &value, const char **out, const char *fn, int argIndex)
    {
        if (!out || !value.isString())
        {
            Error("%s arg %d expects string", fn, argIndex);
            return false;
        }

        *out = value.asString()->chars();
        return true;
    }

    static std::uint64_t allocate_world(WorldRuntime *runtime)
    {
        if (!runtime)
            return 0;

        std::uint64_t id = g_nextWorldId++;
        while (id == 0 || g_worlds.find(id) != g_worlds.end())
            id = g_nextWorldId++;

        g_worlds[id] = runtime;
        return id;
    }

    WorldRuntime *find_world_runtime(std::uint64_t worldId)
    {
        auto it = g_worlds.find(worldId);
        if (it == g_worlds.end())
            return nullptr;

        WorldRuntime *runtime = it->second;
        if (!runtime || !runtime->valid)
            return nullptr;

        return runtime;
    }

    WorldHandle *require_world(void *instance, const char *fn)
    {
        WorldHandle *world = (WorldHandle *)instance;
        if (!world || !world->valid)
        {
            Error("%s: invalid or destroyed ECSWorld", fn);
            return nullptr;
        }

        WorldRuntime *runtime = find_world_runtime(world->worldId);
        if (!runtime)
        {
            Error("%s: invalid or destroyed ECSWorld", fn);
            return nullptr;
        }

        return world;
    }

    EntityHandle *require_entity(void *instance, const char *fn, WorldRuntime **outWorld)
    {
        if (outWorld)
            *outWorld = nullptr;

        EntityHandle *entity = (EntityHandle *)instance;
        if (!entity || !entity->valid || entity->entity == entt::null)
        {
            Error("%s: invalid ECSEntity handle", fn);
            return nullptr;
        }

        WorldRuntime *runtime = find_world_runtime(entity->worldId);
        if (!runtime)
        {
            Error("%s: owner ECSWorld no longer exists", fn);
            return nullptr;
        }

        if (!runtime->registry.valid(entity->entity))
        {
            Error("%s: entity is not alive", fn);
            return nullptr;
        }

        if (outWorld)
            *outWorld = runtime;

        return entity;
    }

    bool read_entity_arg(const Value &value, EntityHandle **out, const char *fn, int argIndex)
    {
        if (!out)
            return false;

        if (!value.isNativeClassInstance())
        {
            Error("%s arg %d expects ECSEntity", fn, argIndex);
            return false;
        }

        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst || inst->klass != g_entityClass || !inst->userData)
        {
            Error("%s arg %d expects ECSEntity", fn, argIndex);
            return false;
        }

        *out = (EntityHandle *)inst->userData;
        return true;
    }

    bool push_entity_handle(Interpreter *vm, std::uint64_t worldId, entt::entity entity)
    {
        if (!vm || !g_entityClass)
            return false;

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst)
            return false;

        EntityHandle *handle = new EntityHandle();
        handle->worldId = worldId;
        handle->entity = entity;
        handle->valid = true;

        inst->klass = g_entityClass;
        inst->userData = handle;
        vm->push(value);
        return true;
    }

    int entity_to_id(entt::entity entity)
    {
        return (int)entt::to_integral(entity);
    }

    entt::entity id_to_entity(int id)
    {
        if (id < 0)
            return entt::null;

        const entt::id_type raw = (entt::id_type)(unsigned int)id;
        return (entt::entity)raw;
    }

    bool is_entity_alive(EntityHandle *entityHandle)
    {
        if (!entityHandle || !entityHandle->valid || entityHandle->entity == entt::null)
            return false;

        WorldRuntime *runtime = find_world_runtime(entityHandle->worldId);
        if (!runtime)
            return false;

        return runtime->registry.valid(entityHandle->entity);
    }

    void destroy_world_runtime(WorldHandle *world)
    {
        if (!world)
            return;

        if (world->valid && world->worldId != 0)
        {
            auto it = g_worlds.find(world->worldId);
            if (it != g_worlds.end())
            {
                WorldRuntime *runtime = it->second;
                g_worlds.erase(it);
                delete runtime;
            }
        }

        world->worldId = 0;
        world->valid = false;
    }

    static void cleanup_all_worlds()
    {
        for (auto &it : g_worlds)
            delete it.second;

        g_worlds.clear();
        g_nextWorldId = 1;
    }

    void cleanup()
    {
        cleanup_all_worlds();
        g_worldClass = nullptr;
        g_entityClass = nullptr;
    }

    static void *world_ctor_raw(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("ECSWorld() expects 0 arguments");
            return nullptr;
        }

        WorldRuntime *runtime = new WorldRuntime();
        runtime->valid = true;

        WorldHandle *handle = new WorldHandle();
        handle->worldId = allocate_world(runtime);
        handle->valid = (handle->worldId != 0);

        if (!handle->valid)
        {
            delete handle;
            delete runtime;
            return nullptr;
        }

        return handle;
    }

    static void world_dtor_raw(Interpreter *vm, void *instance)
    {
        (void)vm;
        WorldHandle *world = (WorldHandle *)instance;
        if (!world)
            return;

        destroy_world_runtime(world);
        delete world;
    }

    static void *entity_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)argCount;
        (void)args;
        Error("ECSEntity cannot be created directly. Use ECSWorld.createEntity().");
        return nullptr;
    }

    static void entity_dtor_raw(Interpreter *vm, void *instance)
    {
        (void)vm;
        EntityHandle *entity = (EntityHandle *)instance;
        delete entity;
    }

    // Referenciadas noutros ficheiros para registar classes.
    void *entt_world_ctor(Interpreter *vm, int argCount, Value *args)
    {
        return world_ctor_raw(vm, argCount, args);
    }

    void entt_world_dtor(Interpreter *vm, void *instance)
    {
        world_dtor_raw(vm, instance);
    }

    void *entt_entity_ctor(Interpreter *vm, int argCount, Value *args)
    {
        return entity_ctor_error(vm, argCount, args);
    }

    void entt_entity_dtor(Interpreter *vm, void *instance)
    {
        entity_dtor_raw(vm, instance);
    }
}
