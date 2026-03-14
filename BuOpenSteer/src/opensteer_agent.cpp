#include "opensteer_core.hpp"

namespace OpenSteerBindings
{
    static void *agent_ctor(Interpreter *vm, int argCount, Value *args)
    {
        AgentHandle *handle = new AgentHandle();
        handle->agent = new BuSteerAgent();
        handle->agent->reset();
        handle->valid = true;

        if (argCount == 1)
        {
            Vector3 position;
            if (!read_vector3_arg(args[0], &position, "SteerAgent()", 1))
            {
                destroy_agent_runtime(handle);
                delete handle;
                return nullptr;
            }
            handle->agent->setPosition(to_opensteer_vec3(position));
        }
        else if (argCount != 0)
        {
            Error("SteerAgent() expects () or (position)");
            destroy_agent_runtime(handle);
            delete handle;
            return nullptr;
        }

        return handle;
    }

    static void agent_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        AgentHandle *handle = (AgentHandle *)instance;
        if (!handle)
            return;

        destroy_agent_runtime(handle);
        delete handle;
    }

    static int agent_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.destroy() expects 0 arguments");
            return 0;
        }

        destroy_agent_runtime((AgentHandle *)instance);
        return 0;
    }

    static int agent_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = (AgentHandle *)instance;
        vm->pushBool(handle != nullptr && handle->valid && handle->agent != nullptr);
        return 1;
    }

    static int agent_reset(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.reset() expects 0 arguments");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.reset()");
        if (!handle)
            return 0;

        handle->agent->reset();
        return 0;
    }

    static int agent_get_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.getPosition() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.getPosition()");
        if (!handle)
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->agent->position())) ? 1 : push_nil1(vm);
    }

    static int agent_set_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.setPosition() expects (Vector3)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.setPosition()");
        if (!handle)
            return 0;

        Vector3 position;
        if (!read_vector3_arg(args[0], &position, "SteerAgent.setPosition()", 1))
            return 0;

        handle->agent->setPosition(to_opensteer_vec3(position));
        return 0;
    }

    static int agent_get_forward(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.getForward() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.getForward()");
        if (!handle)
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->agent->forward())) ? 1 : push_nil1(vm);
    }

    static int agent_set_forward(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.setForward() expects (Vector3)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.setForward()");
        if (!handle)
            return 0;

        Vector3 forward;
        if (!read_vector3_arg(args[0], &forward, "SteerAgent.setForward()", 1))
            return 0;

        OpenSteer::Vec3 newForward = to_opensteer_vec3(forward);
        if (newForward.lengthSquared() <= 0.000001f)
            return 0;

        handle->agent->regenerateOrthonormalBasis(newForward);
        return 0;
    }

    static int agent_randomize_heading_xz(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.randomizeHeadingXZ() expects 0 arguments");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.randomizeHeadingXZ()");
        if (!handle)
            return 0;

        handle->agent->randomizeHeadingOnXZPlane();
        return 0;
    }

    static int agent_get_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.getVelocity() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.getVelocity()");
        if (!handle)
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->agent->velocity())) ? 1 : push_nil1(vm);
    }

    static int agent_get_speed(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.getSpeed() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.getSpeed()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->agent->speed()));
        return 1;
    }

    static int agent_set_speed(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.setSpeed() expects (speed)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.setSpeed()");
        if (!handle)
            return 0;

        double speed = 0.0;
        if (!read_number_arg(args[0], &speed, "SteerAgent.setSpeed()", 1))
            return 0;

        handle->agent->setSpeed((float)speed);
        return 0;
    }

    static int agent_get_mass(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.getMass() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.getMass()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->agent->mass()));
        return 1;
    }

    static int agent_set_mass(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.setMass() expects (mass)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.setMass()");
        if (!handle)
            return 0;

        double mass = 0.0;
        if (!read_number_arg(args[0], &mass, "SteerAgent.setMass()", 1))
            return 0;

        handle->agent->setMass((float)mass);
        return 0;
    }

    static int agent_get_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.getRadius() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.getRadius()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->agent->radius()));
        return 1;
    }

    static int agent_set_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.setRadius() expects (radius)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.setRadius()");
        if (!handle)
            return 0;

        double radius = 0.0;
        if (!read_number_arg(args[0], &radius, "SteerAgent.setRadius()", 1))
            return 0;

        handle->agent->setRadius((float)radius);
        return 0;
    }

    static int agent_get_max_force(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.getMaxForce() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.getMaxForce()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->agent->maxForce()));
        return 1;
    }

    static int agent_set_max_force(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.setMaxForce() expects (maxForce)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.setMaxForce()");
        if (!handle)
            return 0;

        double maxForce = 0.0;
        if (!read_number_arg(args[0], &maxForce, "SteerAgent.setMaxForce()", 1))
            return 0;

        handle->agent->setMaxForce((float)maxForce);
        return 0;
    }

    static int agent_get_max_speed(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerAgent.getMaxSpeed() expects 0 arguments");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.getMaxSpeed()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->agent->maxSpeed()));
        return 1;
    }

    static int agent_set_max_speed(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.setMaxSpeed() expects (maxSpeed)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.setMaxSpeed()");
        if (!handle)
            return 0;

        double maxSpeed = 0.0;
        if (!read_number_arg(args[0], &maxSpeed, "SteerAgent.setMaxSpeed()", 1))
            return 0;

        handle->agent->setMaxSpeed((float)maxSpeed);
        return 0;
    }

    static int agent_predict_future_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.predictFuturePosition() expects (predictionTime)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.predictFuturePosition()");
        if (!handle)
            return push_nil1(vm);

        double predictionTime = 0.0;
        if (!read_number_arg(args[0], &predictionTime, "SteerAgent.predictFuturePosition()", 1))
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->agent->predictFuturePosition((float)predictionTime))) ? 1 : push_nil1(vm);
    }

    static int agent_steer_for_seek(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.seek() expects (target)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.seek()");
        if (!handle)
            return push_nil1(vm);

        Vector3 target;
        if (!read_vector3_arg(args[0], &target, "SteerAgent.seek()", 1))
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->agent->steerForSeek(to_opensteer_vec3(target)))) ? 1 : push_nil1(vm);
    }

    static int agent_steer_for_flee(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.flee() expects (target)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.flee()");
        if (!handle)
            return push_nil1(vm);

        Vector3 target;
        if (!read_vector3_arg(args[0], &target, "SteerAgent.flee()", 1))
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->agent->steerForFlee(to_opensteer_vec3(target)))) ? 1 : push_nil1(vm);
    }

    static int agent_steer_for_wander(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.wander() expects (dt)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.wander()");
        if (!handle)
            return push_nil1(vm);

        double dt = 0.0;
        if (!read_number_arg(args[0], &dt, "SteerAgent.wander()", 1))
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->agent->steerForWander((float)dt))) ? 1 : push_nil1(vm);
    }

    static int agent_steer_for_target_speed(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerAgent.targetSpeed() expects (speed)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.targetSpeed()");
        if (!handle)
            return push_nil1(vm);

        double speed = 0.0;
        if (!read_number_arg(args[0], &speed, "SteerAgent.targetSpeed()", 1))
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->agent->steerForTargetSpeed((float)speed))) ? 1 : push_nil1(vm);
    }

    static int agent_steer_for_arrival_method(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("SteerAgent.arrive() expects (target, slowingDistance)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.arrive()");
        if (!handle)
            return push_nil1(vm);

        Vector3 target;
        double slowingDistance = 0.0;
        if (!read_vector3_arg(args[0], &target, "SteerAgent.arrive()", 1) ||
            !read_number_arg(args[1], &slowingDistance, "SteerAgent.arrive()", 2))
        {
            return push_nil1(vm);
        }

        return push_vector3(vm,
                            from_opensteer_vec3(steer_for_arrival(*handle->agent,
                                                                  to_opensteer_vec3(target),
                                                                  (float)slowingDistance)))
                   ? 1
                   : push_nil1(vm);
    }

    static int agent_steer_for_pursuit(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 2)
        {
            Error("SteerAgent.pursuit() expects (other[, maxPredictionTime])");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.pursuit()");
        if (!handle)
            return push_nil1(vm);

        AgentHandle *other = require_agent(args[0].isNativeClassInstance() ? args[0].asNativeClassInstance()->userData : nullptr,
                                           "SteerAgent.pursuit()");
        if (!other)
            return push_nil1(vm);

        OpenSteer::Vec3 result = OpenSteer::Vec3::zero;
        if (argCount == 2)
        {
            double maxPredictionTime = 0.0;
            if (!read_number_arg(args[1], &maxPredictionTime, "SteerAgent.pursuit()", 2))
                return push_nil1(vm);
            result = handle->agent->steerForPursuit(*other->agent, (float)maxPredictionTime);
        }
        else
            result = handle->agent->steerForPursuit(*other->agent);

        return push_vector3(vm, from_opensteer_vec3(result)) ? 1 : push_nil1(vm);
    }

    static int agent_steer_for_evasion(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("SteerAgent.evasion() expects (other, maxPredictionTime)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.evasion()");
        if (!handle)
            return push_nil1(vm);

        AgentHandle *other = require_agent(args[0].isNativeClassInstance() ? args[0].asNativeClassInstance()->userData : nullptr,
                                           "SteerAgent.evasion()");
        if (!other)
            return push_nil1(vm);

        double maxPredictionTime = 0.0;
        if (!read_number_arg(args[1], &maxPredictionTime, "SteerAgent.evasion()", 2))
            return push_nil1(vm);

        return push_vector3(vm,
                            from_opensteer_vec3(handle->agent->steerForEvasion(*other->agent, (float)maxPredictionTime)))
                   ? 1
                   : push_nil1(vm);
    }

    static int agent_steer_to_avoid_sphere(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("SteerAgent.avoidSphere() expects (minTimeToCollision, obstacle)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.avoidSphere()");
        if (!handle)
            return push_nil1(vm);

        double minTimeToCollision = 0.0;
        if (!read_number_arg(args[0], &minTimeToCollision, "SteerAgent.avoidSphere()", 1))
            return push_nil1(vm);

        SphereObstacleHandle *obstacle = require_sphere_obstacle(args[1].isNativeClassInstance() ? args[1].asNativeClassInstance()->userData : nullptr,
                                                                 "SteerAgent.avoidSphere()");
        if (!obstacle)
            return push_nil1(vm);

        return push_vector3(vm,
                            from_opensteer_vec3(handle->agent->steerToAvoidObstacle((float)minTimeToCollision,
                                                                                    *obstacle->obstacle)))
                   ? 1
                   : push_nil1(vm);
    }

    static int agent_steer_for_separation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("SteerAgent.separation() expects (maxDistance, cosMaxAngle, agents)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.separation()");
        if (!handle)
            return push_nil1(vm);

        double maxDistance = 0.0;
        double cosMaxAngle = 0.0;
        OpenSteer::AVGroup group;
        if (!read_number_arg(args[0], &maxDistance, "SteerAgent.separation()", 1) ||
            !read_number_arg(args[1], &cosMaxAngle, "SteerAgent.separation()", 2) ||
            !read_agent_array(args[2], &group, "SteerAgent.separation()", 3))
        {
            return push_nil1(vm);
        }

        return push_vector3(vm,
                            from_opensteer_vec3(handle->agent->steerForSeparation((float)maxDistance,
                                                                                  (float)cosMaxAngle,
                                                                                  group)))
                   ? 1
                   : push_nil1(vm);
    }

    static int agent_steer_for_alignment(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("SteerAgent.alignment() expects (maxDistance, cosMaxAngle, agents)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.alignment()");
        if (!handle)
            return push_nil1(vm);

        double maxDistance = 0.0;
        double cosMaxAngle = 0.0;
        OpenSteer::AVGroup group;
        if (!read_number_arg(args[0], &maxDistance, "SteerAgent.alignment()", 1) ||
            !read_number_arg(args[1], &cosMaxAngle, "SteerAgent.alignment()", 2) ||
            !read_agent_array(args[2], &group, "SteerAgent.alignment()", 3))
        {
            return push_nil1(vm);
        }

        return push_vector3(vm,
                            from_opensteer_vec3(handle->agent->steerForAlignment((float)maxDistance,
                                                                                 (float)cosMaxAngle,
                                                                                 group)))
                   ? 1
                   : push_nil1(vm);
    }

    static int agent_steer_for_cohesion(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("SteerAgent.cohesion() expects (maxDistance, cosMaxAngle, agents)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.cohesion()");
        if (!handle)
            return push_nil1(vm);

        double maxDistance = 0.0;
        double cosMaxAngle = 0.0;
        OpenSteer::AVGroup group;
        if (!read_number_arg(args[0], &maxDistance, "SteerAgent.cohesion()", 1) ||
            !read_number_arg(args[1], &cosMaxAngle, "SteerAgent.cohesion()", 2) ||
            !read_agent_array(args[2], &group, "SteerAgent.cohesion()", 3))
        {
            return push_nil1(vm);
        }

        return push_vector3(vm,
                            from_opensteer_vec3(handle->agent->steerForCohesion((float)maxDistance,
                                                                                (float)cosMaxAngle,
                                                                                group)))
                   ? 1
                   : push_nil1(vm);
    }

    static int agent_steer_to_avoid_neighbors(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("SteerAgent.avoidNeighbors() expects (minTimeToCollision, agents)");
            return push_nil1(vm);
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.avoidNeighbors()");
        if (!handle)
            return push_nil1(vm);

        double minTimeToCollision = 0.0;
        OpenSteer::AVGroup group;
        if (!read_number_arg(args[0], &minTimeToCollision, "SteerAgent.avoidNeighbors()", 1) ||
            !read_agent_array(args[1], &group, "SteerAgent.avoidNeighbors()", 2))
        {
            return push_nil1(vm);
        }

        return push_vector3(vm,
                            from_opensteer_vec3(handle->agent->steerToAvoidNeighbors((float)minTimeToCollision,
                                                                                     group)))
                   ? 1
                   : push_nil1(vm);
    }

    static int agent_apply_steering_force(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("SteerAgent.applySteeringForce() expects (force, dt)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.applySteeringForce()");
        if (!handle)
            return 0;

        Vector3 force;
        double dt = 0.0;
        if (!read_vector3_arg(args[0], &force, "SteerAgent.applySteeringForce()", 1) ||
            !read_number_arg(args[1], &dt, "SteerAgent.applySteeringForce()", 2))
        {
            return 0;
        }

        handle->agent->applySteeringForce(to_opensteer_vec3(force), (float)dt);
        return 0;
    }

    static int agent_apply_braking_force(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("SteerAgent.applyBrakingForce() expects (rate, dt)");
            return 0;
        }

        AgentHandle *handle = require_agent(instance, "SteerAgent.applyBrakingForce()");
        if (!handle)
            return 0;

        double rate = 0.0;
        double dt = 0.0;
        if (!read_number_arg(args[0], &rate, "SteerAgent.applyBrakingForce()", 1) ||
            !read_number_arg(args[1], &dt, "SteerAgent.applyBrakingForce()", 2))
        {
            return 0;
        }

        handle->agent->applyBrakingForce((float)rate, (float)dt);
        return 0;
    }

    void register_agent(Interpreter &vm)
    {
        g_agentClass = vm.registerNativeClass("SteerAgent", agent_ctor, agent_dtor, -1, false);

        vm.addNativeMethod(g_agentClass, "destroy", agent_destroy);
        vm.addNativeMethod(g_agentClass, "isValid", agent_is_valid);
        vm.addNativeMethod(g_agentClass, "reset", agent_reset);
        vm.addNativeMethod(g_agentClass, "getPosition", agent_get_position);
        vm.addNativeMethod(g_agentClass, "setPosition", agent_set_position);
        vm.addNativeMethod(g_agentClass, "getForward", agent_get_forward);
        vm.addNativeMethod(g_agentClass, "setForward", agent_set_forward);
        vm.addNativeMethod(g_agentClass, "randomizeHeadingXZ", agent_randomize_heading_xz);
        vm.addNativeMethod(g_agentClass, "getVelocity", agent_get_velocity);
        vm.addNativeMethod(g_agentClass, "getSpeed", agent_get_speed);
        vm.addNativeMethod(g_agentClass, "setSpeed", agent_set_speed);
        vm.addNativeMethod(g_agentClass, "getMass", agent_get_mass);
        vm.addNativeMethod(g_agentClass, "setMass", agent_set_mass);
        vm.addNativeMethod(g_agentClass, "getRadius", agent_get_radius);
        vm.addNativeMethod(g_agentClass, "setRadius", agent_set_radius);
        vm.addNativeMethod(g_agentClass, "getMaxForce", agent_get_max_force);
        vm.addNativeMethod(g_agentClass, "setMaxForce", agent_set_max_force);
        vm.addNativeMethod(g_agentClass, "getMaxSpeed", agent_get_max_speed);
        vm.addNativeMethod(g_agentClass, "setMaxSpeed", agent_set_max_speed);
        vm.addNativeMethod(g_agentClass, "predictFuturePosition", agent_predict_future_position);
        vm.addNativeMethod(g_agentClass, "seek", agent_steer_for_seek);
        vm.addNativeMethod(g_agentClass, "flee", agent_steer_for_flee);
        vm.addNativeMethod(g_agentClass, "wander", agent_steer_for_wander);
        vm.addNativeMethod(g_agentClass, "targetSpeed", agent_steer_for_target_speed);
        vm.addNativeMethod(g_agentClass, "arrive", agent_steer_for_arrival_method);
        vm.addNativeMethod(g_agentClass, "pursuit", agent_steer_for_pursuit);
        vm.addNativeMethod(g_agentClass, "evasion", agent_steer_for_evasion);
        vm.addNativeMethod(g_agentClass, "avoidSphere", agent_steer_to_avoid_sphere);
        vm.addNativeMethod(g_agentClass, "separation", agent_steer_for_separation);
        vm.addNativeMethod(g_agentClass, "alignment", agent_steer_for_alignment);
        vm.addNativeMethod(g_agentClass, "cohesion", agent_steer_for_cohesion);
        vm.addNativeMethod(g_agentClass, "avoidNeighbors", agent_steer_to_avoid_neighbors);
        vm.addNativeMethod(g_agentClass, "applySteeringForce", agent_apply_steering_force);
        vm.addNativeMethod(g_agentClass, "applyBrakingForce", agent_apply_braking_force);
    }
}
