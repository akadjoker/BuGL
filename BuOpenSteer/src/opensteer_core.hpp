#pragma once

#include "bindings.hpp"
#include "../../main/src/raymath.h"

#include <OpenSteer/SimpleVehicle.h>
#include <OpenSteer/Obstacle.h>

namespace OpenSteerBindings
{
    class BuSteerAgent final : public OpenSteer::SimpleVehicle
    {
    public:
        void update(const float currentTime, const float elapsedTime) override
        {
            (void)currentTime;
            (void)elapsedTime;
        }
    };

    struct AgentHandle
    {
        BuSteerAgent *agent = nullptr;
        bool valid = false;
    };

    struct SphereObstacleHandle
    {
        OpenSteer::SphereObstacle *obstacle = nullptr;
        bool valid = false;
    };

    extern NativeClassDef *g_agentClass;
    extern NativeClassDef *g_sphereObstacleClass;
    extern NativeStructDef *g_vector3Def;

    int push_nil1(Interpreter *vm);

    NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name);
    bool read_number_arg(const Value &value, double *out, const char *fn, int argIndex);
    bool read_vector3_arg(const Value &value, Vector3 *out, const char *fn, int argIndex);
    bool push_vector3(Interpreter *vm, const Vector3 &value);

    OpenSteer::Vec3 to_opensteer_vec3(const Vector3 &value);
    Vector3 from_opensteer_vec3(const OpenSteer::Vec3 &value);

    AgentHandle *require_agent(void *instance, const char *fn);
    SphereObstacleHandle *require_sphere_obstacle(void *instance, const char *fn);

    void destroy_agent_runtime(AgentHandle *handle);
    void destroy_sphere_obstacle_runtime(SphereObstacleHandle *handle);

    bool push_agent_handle(Interpreter *vm, AgentHandle *handle);
    bool push_sphere_obstacle_handle(Interpreter *vm, SphereObstacleHandle *handle);
    bool read_agent_array(const Value &value, OpenSteer::AVGroup *out, const char *fn, int argIndex);

    OpenSteer::Vec3 steer_for_arrival(BuSteerAgent &agent, const OpenSteer::Vec3 &target, float slowingDistance);

    void register_agent(Interpreter &vm);
    void register_obstacle(Interpreter &vm);
}
