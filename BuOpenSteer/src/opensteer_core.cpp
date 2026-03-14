#include "opensteer_core.hpp"

namespace OpenSteerBindings
{
    NativeClassDef *g_agentClass = nullptr;
    NativeClassDef *g_sphereObstacleClass = nullptr;
    NativeStructDef *g_vector3Def = nullptr;

    int push_nil1(Interpreter *vm)
    {
        vm->pushNil();
        return 1;
    }

    NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name)
    {
        if (!vm || !name)
            return nullptr;

        Value structValue;
        if (!vm->tryGetGlobal(name, &structValue) || !structValue.isNativeStruct())
            return nullptr;

        Value instanceValue = vm->createNativeStruct(structValue.asNativeStructId(), 0, nullptr);
        NativeStructInstance *inst = instanceValue.asNativeStructInstance();
        return inst ? inst->def : nullptr;
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

    bool read_vector3_arg(const Value &value, Vector3 *out, const char *fn, int argIndex)
    {
        if (!out || !value.isNativeStructInstance())
        {
            Error("%s arg %d expects Vector3", fn, argIndex);
            return false;
        }

        NativeStructInstance *inst = value.asNativeStructInstance();
        if (!inst || !inst->data || (g_vector3Def != nullptr && inst->def != g_vector3Def))
        {
            Error("%s arg %d expects Vector3", fn, argIndex);
            return false;
        }

        *out = *(Vector3 *)inst->data;
        return true;
    }

    bool push_vector3(Interpreter *vm, const Vector3 &value)
    {
        if (!vm || !g_vector3Def)
        {
            Error("Vector3 struct is not available");
            return false;
        }

        Value out = vm->createNativeStruct(g_vector3Def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;

        *(Vector3 *)inst->data = value;
        vm->push(out);
        return true;
    }

    OpenSteer::Vec3 to_opensteer_vec3(const Vector3 &value)
    {
        return OpenSteer::Vec3(value.x, value.y, value.z);
    }

    Vector3 from_opensteer_vec3(const OpenSteer::Vec3 &value)
    {
        Vector3 out;
        out.x = value.x;
        out.y = value.y;
        out.z = value.z;
        return out;
    }

    AgentHandle *require_agent(void *instance, const char *fn)
    {
        AgentHandle *handle = (AgentHandle *)instance;
        if (!handle || !handle->valid || !handle->agent)
        {
            Error("%s on invalid SteerAgent", fn);
            return nullptr;
        }
        return handle;
    }

    SphereObstacleHandle *require_sphere_obstacle(void *instance, const char *fn)
    {
        SphereObstacleHandle *handle = (SphereObstacleHandle *)instance;
        if (!handle || !handle->valid || !handle->obstacle)
        {
            Error("%s on invalid SteerSphereObstacle", fn);
            return nullptr;
        }
        return handle;
    }

    void destroy_agent_runtime(AgentHandle *handle)
    {
        if (!handle || !handle->valid)
            return;

        delete handle->agent;
        handle->agent = nullptr;
        handle->valid = false;
    }

    void destroy_sphere_obstacle_runtime(SphereObstacleHandle *handle)
    {
        if (!handle || !handle->valid)
            return;

        delete handle->obstacle;
        handle->obstacle = nullptr;
        handle->valid = false;
    }

    bool push_agent_handle(Interpreter *vm, AgentHandle *handle)
    {
        if (!vm || !handle || !g_agentClass)
            return false;

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst)
            return false;

        inst->klass = g_agentClass;
        inst->userData = handle;
        vm->push(value);
        return true;
    }

    bool push_sphere_obstacle_handle(Interpreter *vm, SphereObstacleHandle *handle)
    {
        if (!vm || !handle || !g_sphereObstacleClass)
            return false;

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst)
            return false;

        inst->klass = g_sphereObstacleClass;
        inst->userData = handle;
        vm->push(value);
        return true;
    }

    bool read_agent_array(const Value &value, OpenSteer::AVGroup *out, const char *fn, int argIndex)
    {
        if (!out || !value.isArray())
        {
            Error("%s arg %d expects array of SteerAgent", fn, argIndex);
            return false;
        }

        ArrayInstance *arr = value.asArray();
        if (!arr)
        {
            Error("%s arg %d expects array of SteerAgent", fn, argIndex);
            return false;
        }

        out->clear();
        out->reserve(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); ++i)
        {
            const Value &item = arr->values[i];
            if (!item.isNativeClassInstance())
            {
                Error("%s arg %d item %d expects SteerAgent", fn, argIndex, (int)i + 1);
                return false;
            }

            NativeClassInstance *inst = item.asNativeClassInstance();
            if (!inst || inst->klass != g_agentClass || !inst->userData)
            {
                Error("%s arg %d item %d expects SteerAgent", fn, argIndex, (int)i + 1);
                return false;
            }

            AgentHandle *handle = (AgentHandle *)inst->userData;
            if (!handle || !handle->valid || !handle->agent)
            {
                Error("%s arg %d item %d is invalid", fn, argIndex, (int)i + 1);
                return false;
            }

            out->push_back(handle->agent);
        }

        return true;
    }

    OpenSteer::Vec3 steer_for_arrival(BuSteerAgent &agent, const OpenSteer::Vec3 &target, float slowingDistance)
    {
        if (slowingDistance <= 0.0001f)
            slowingDistance = 0.0001f;

        const OpenSteer::Vec3 offset = target - agent.position();
        const float distance = offset.length();
        if (distance <= 0.0001f)
            return OpenSteer::Vec3::zero;

        const float rampedSpeed = agent.maxSpeed() * (distance / slowingDistance);
        const float clippedSpeed = OpenSteer::minXXX(rampedSpeed, agent.maxSpeed());
        const OpenSteer::Vec3 desiredVelocity = offset * (clippedSpeed / distance);
        return (desiredVelocity - agent.velocity()).truncateLength(agent.maxForce());
    }
}
