#pragma once

#include "bindings.hpp"

#include <entt/entt.hpp>

#include <cstdint>
#include <string>

namespace EnTTBindings
{
    struct TagComponent
    {
        std::string value;
    };

    struct Transform2DComponent
    {
        float x = 0.0f;
        float y = 0.0f;
        float rotation = 0.0f;
        float sx = 1.0f;
        float sy = 1.0f;
    };

    struct Velocity2DComponent
    {
        float vx = 0.0f;
        float vy = 0.0f;
    };

    struct Transform3DComponent
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float rx = 0.0f;
        float ry = 0.0f;
        float rz = 0.0f;
        float sx = 1.0f;
        float sy = 1.0f;
        float sz = 1.0f;
    };

    struct WorldRuntime
    {
        entt::registry registry;
        bool valid = true;
    };

    struct WorldHandle
    {
        std::uint64_t worldId = 0;
        bool valid = false;
    };

    struct EntityHandle
    {
        std::uint64_t worldId = 0;
        entt::entity entity = entt::null;
        bool valid = false;
    };

    extern NativeClassDef *g_worldClass;
    extern NativeClassDef *g_entityClass;

    int push_nil1(Interpreter *vm);

    bool read_number_arg(const Value &value, double *out, const char *fn, int argIndex);
    bool read_string_arg(const Value &value, const char **out, const char *fn, int argIndex);

    WorldRuntime *find_world_runtime(std::uint64_t worldId);
    WorldHandle *require_world(void *instance, const char *fn);
    EntityHandle *require_entity(void *instance, const char *fn, WorldRuntime **outWorld);

    bool read_entity_arg(const Value &value, EntityHandle **out, const char *fn, int argIndex);
    bool push_entity_handle(Interpreter *vm, std::uint64_t worldId, entt::entity entity);

    int entity_to_id(entt::entity entity);
    entt::entity id_to_entity(int id);

    bool is_entity_alive(EntityHandle *entityHandle);
    void destroy_world_runtime(WorldHandle *world);

    void register_world(Interpreter &vm);
    void register_entity(Interpreter &vm);
}
