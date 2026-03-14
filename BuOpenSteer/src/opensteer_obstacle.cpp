#include "opensteer_core.hpp"

namespace OpenSteerBindings
{
    static void *sphere_obstacle_ctor(Interpreter *vm, int argCount, Value *args)
    {
        SphereObstacleHandle *handle = new SphereObstacleHandle();
        handle->obstacle = new OpenSteer::SphereObstacle();
        handle->valid = true;

        if (argCount == 2)
        {
            double radius = 0.0;
            Vector3 center;
            if (!read_number_arg(args[0], &radius, "SteerSphereObstacle()", 1) ||
                !read_vector3_arg(args[1], &center, "SteerSphereObstacle()", 2))
            {
                destroy_sphere_obstacle_runtime(handle);
                delete handle;
                return nullptr;
            }

            handle->obstacle->radius = (float)radius;
            handle->obstacle->center = to_opensteer_vec3(center);
        }
        else if (argCount != 0)
        {
            Error("SteerSphereObstacle() expects () or (radius, center)");
            destroy_sphere_obstacle_runtime(handle);
            delete handle;
            return nullptr;
        }

        return handle;
    }

    static void sphere_obstacle_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        SphereObstacleHandle *handle = (SphereObstacleHandle *)instance;
        if (!handle)
            return;

        destroy_sphere_obstacle_runtime(handle);
        delete handle;
    }

    static int sphere_obstacle_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("SteerSphereObstacle.destroy() expects 0 arguments");
            return 0;
        }

        destroy_sphere_obstacle_runtime((SphereObstacleHandle *)instance);
        return 0;
    }

    static int sphere_obstacle_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerSphereObstacle.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        SphereObstacleHandle *handle = (SphereObstacleHandle *)instance;
        vm->pushBool(handle != nullptr && handle->valid && handle->obstacle != nullptr);
        return 1;
    }

    static int sphere_obstacle_get_center(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerSphereObstacle.getCenter() expects 0 arguments");
            return push_nil1(vm);
        }

        SphereObstacleHandle *handle = require_sphere_obstacle(instance, "SteerSphereObstacle.getCenter()");
        if (!handle)
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->obstacle->center)) ? 1 : push_nil1(vm);
    }

    static int sphere_obstacle_set_center(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerSphereObstacle.setCenter() expects (Vector3)");
            return 0;
        }

        SphereObstacleHandle *handle = require_sphere_obstacle(instance, "SteerSphereObstacle.setCenter()");
        if (!handle)
            return 0;

        Vector3 center;
        if (!read_vector3_arg(args[0], &center, "SteerSphereObstacle.setCenter()", 1))
            return 0;

        handle->obstacle->center = to_opensteer_vec3(center);
        return 0;
    }

    static int sphere_obstacle_get_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerSphereObstacle.getRadius() expects 0 arguments");
            return push_nil1(vm);
        }

        SphereObstacleHandle *handle = require_sphere_obstacle(instance, "SteerSphereObstacle.getRadius()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->obstacle->radius));
        return 1;
    }

    static int sphere_obstacle_set_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerSphereObstacle.setRadius() expects (radius)");
            return 0;
        }

        SphereObstacleHandle *handle = require_sphere_obstacle(instance, "SteerSphereObstacle.setRadius()");
        if (!handle)
            return 0;

        double radius = 0.0;
        if (!read_number_arg(args[0], &radius, "SteerSphereObstacle.setRadius()", 1))
            return 0;

        handle->obstacle->radius = (float)radius;
        return 0;
    }

    void register_obstacle(Interpreter &vm)
    {
        g_sphereObstacleClass = vm.registerNativeClass("SteerSphereObstacle", sphere_obstacle_ctor, sphere_obstacle_dtor, -1, false);

        vm.addNativeMethod(g_sphereObstacleClass, "destroy", sphere_obstacle_destroy);
        vm.addNativeMethod(g_sphereObstacleClass, "isValid", sphere_obstacle_is_valid);
        vm.addNativeMethod(g_sphereObstacleClass, "getCenter", sphere_obstacle_get_center);
        vm.addNativeMethod(g_sphereObstacleClass, "setCenter", sphere_obstacle_set_center);
        vm.addNativeMethod(g_sphereObstacleClass, "getRadius", sphere_obstacle_get_radius);
        vm.addNativeMethod(g_sphereObstacleClass, "setRadius", sphere_obstacle_set_radius);
    }
}
