#include "opensteer_core.hpp"

namespace OpenSteerBindings
{
    static bool validate_points_count(const std::vector<OpenSteer::Vec3> &points, const char *fn)
    {
        if (points.size() < 2)
        {
            Error("%s requires at least 2 points", fn);
            return false;
        }
        return true;
    }

    static void *pathway_ctor(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount < 2 || argCount > 3)
        {
            Error("SteerPathway(points, radius [, cyclic])");
            return nullptr;
        }

        std::vector<OpenSteer::Vec3> points;
        if (!read_vector3_array_arg(args[0], &points, "SteerPathway()", 1))
            return nullptr;

        if (!validate_points_count(points, "SteerPathway()"))
            return nullptr;

        double radius = 0.0;
        if (!read_number_arg(args[1], &radius, "SteerPathway()", 2))
            return nullptr;

        bool cyclic = false;
        if (argCount == 3 && !read_boolish_arg(args[2], &cyclic, "SteerPathway()", 3))
            return nullptr;

        if (radius <= 0.0)
            radius = 0.001;

        PathwayHandle *handle = new PathwayHandle();
        handle->pathway = new OpenSteer::PolylineSegmentedPathwaySingleRadius(
            (OpenSteer::PolylineSegmentedPathwaySingleRadius::size_type)points.size(),
            points.data(),
            (float)radius,
            cyclic);
        handle->valid = true;
        return handle;
    }

    static void pathway_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        PathwayHandle *handle = (PathwayHandle *)instance;
        if (!handle)
            return;

        destroy_pathway_runtime(handle);
        delete handle;
    }

    static int pathway_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("SteerPathway.destroy() expects 0 arguments");
            return 0;
        }

        destroy_pathway_runtime((PathwayHandle *)instance);
        return 0;
    }

    static int pathway_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerPathway.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        PathwayHandle *handle = (PathwayHandle *)instance;
        vm->pushBool(handle != nullptr && handle->valid && handle->pathway != nullptr && handle->pathway->isValid());
        return 1;
    }

    static int pathway_set_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount < 2 || argCount > 3)
        {
            Error("SteerPathway.setPath(points, radius [, cyclic])");
            return 0;
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.setPath()");
        if (!handle)
            return 0;

        std::vector<OpenSteer::Vec3> points;
        if (!read_vector3_array_arg(args[0], &points, "SteerPathway.setPath()", 1))
            return 0;

        if (!validate_points_count(points, "SteerPathway.setPath()"))
            return 0;

        double radius = 0.0;
        if (!read_number_arg(args[1], &radius, "SteerPathway.setPath()", 2))
            return 0;

        bool cyclic = false;
        if (argCount == 3 && !read_boolish_arg(args[2], &cyclic, "SteerPathway.setPath()", 3))
            return 0;

        if (radius <= 0.0)
            radius = 0.001;

        handle->pathway->setPathway(
            (OpenSteer::PolylineSegmentedPathwaySingleRadius::size_type)points.size(),
            points.data(),
            (float)radius,
            cyclic);
        return 0;
    }

    static int pathway_get_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerPathway.getRadius() expects 0 arguments");
            return push_nil1(vm);
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.getRadius()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->pathway->radius()));
        return 1;
    }

    static int pathway_set_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("SteerPathway.setRadius(radius)");
            return 0;
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.setRadius()");
        if (!handle)
            return 0;

        double radius = 0.0;
        if (!read_number_arg(args[0], &radius, "SteerPathway.setRadius()", 1))
            return 0;

        if (radius <= 0.0)
            radius = 0.001;

        handle->pathway->setRadius((float)radius);
        return 0;
    }

    static int pathway_length(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerPathway.length() expects 0 arguments");
            return push_nil1(vm);
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.length()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->pathway->length()));
        return 1;
    }

    static int pathway_is_cyclic(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerPathway.isCyclic() expects 0 arguments");
            return push_nil1(vm);
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.isCyclic()");
        if (!handle)
            return push_nil1(vm);

        vm->pushBool(handle->pathway->isCyclic());
        return 1;
    }

    static int pathway_point_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("SteerPathway.getPointCount() expects 0 arguments");
            return push_nil1(vm);
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.getPointCount()");
        if (!handle)
            return push_nil1(vm);

        vm->push(vm->makeInt((int)handle->pathway->pointCount()));
        return 1;
    }

    static int pathway_get_point(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerPathway.getPoint(index)");
            return push_nil1(vm);
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.getPoint()");
        if (!handle)
            return push_nil1(vm);

        double idx = 0.0;
        if (!read_number_arg(args[0], &idx, "SteerPathway.getPoint()", 1))
            return push_nil1(vm);

        int i = (int)idx;
        int count = (int)handle->pathway->pointCount();
        if (i < 0 || i >= count)
            return push_nil1(vm);

        return push_vector3(vm, from_opensteer_vec3(handle->pathway->point((OpenSteer::PolylineSegmentedPathwaySingleRadius::size_type)i))) ? 1 : push_nil1(vm);
    }

    static int pathway_map_point(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerPathway.mapPoint(point)");
            return push_nil1(vm);
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.mapPoint()");
        if (!handle)
            return push_nil1(vm);

        Vector3 input;
        if (!read_vector3_arg(args[0], &input, "SteerPathway.mapPoint()", 1))
            return push_nil1(vm);

        OpenSteer::Vec3 tangent;
        float outside = 0.0f;
        OpenSteer::Vec3 onPath = handle->pathway->mapPointToPath(to_opensteer_vec3(input), tangent, outside);

        if (!g_vector3Def)
            return push_nil1(vm);

        Value outVal = vm->makeArray();
        ArrayInstance *outArr = outVal.asArray();
        if (!outArr)
            return push_nil1(vm);

        Value onPathVal = vm->createNativeStruct(g_vector3Def->id, 0, nullptr);
        Value tangentVal = vm->createNativeStruct(g_vector3Def->id, 0, nullptr);
        NativeStructInstance *onPathInst = onPathVal.asNativeStructInstance();
        NativeStructInstance *tangentInst = tangentVal.asNativeStructInstance();
        if (!onPathInst || !onPathInst->data || !tangentInst || !tangentInst->data)
            return push_nil1(vm);

        *(Vector3 *)onPathInst->data = from_opensteer_vec3(onPath);
        *(Vector3 *)tangentInst->data = from_opensteer_vec3(tangent);

        outArr->values.push(onPathVal);
        outArr->values.push(tangentVal);
        outArr->values.push(vm->makeDouble(outside));

        vm->push(outVal);
        return 1;
    }

    static int pathway_map_distance_to_point(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerPathway.mapDistanceToPoint(distance)");
            return push_nil1(vm);
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.mapDistanceToPoint()");
        if (!handle)
            return push_nil1(vm);

        double distance = 0.0;
        if (!read_number_arg(args[0], &distance, "SteerPathway.mapDistanceToPoint()", 1))
            return push_nil1(vm);

        OpenSteer::Vec3 point = handle->pathway->mapPathDistanceToPoint((float)distance);
        return push_vector3(vm, from_opensteer_vec3(point)) ? 1 : push_nil1(vm);
    }

    static int pathway_map_point_to_distance(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("SteerPathway.mapPointToDistance(point)");
            return push_nil1(vm);
        }

        PathwayHandle *handle = require_pathway(instance, "SteerPathway.mapPointToDistance()");
        if (!handle)
            return push_nil1(vm);

        Vector3 input;
        if (!read_vector3_arg(args[0], &input, "SteerPathway.mapPointToDistance()", 1))
            return push_nil1(vm);

        vm->push(vm->makeDouble(handle->pathway->mapPointToPathDistance(to_opensteer_vec3(input))));
        return 1;
    }

    void register_pathway(Interpreter &vm)
    {
        g_pathwayClass = vm.registerNativeClass("SteerPathway", pathway_ctor, pathway_dtor, -1, false);

        vm.addNativeMethod(g_pathwayClass, "destroy", pathway_destroy);
        vm.addNativeMethod(g_pathwayClass, "isValid", pathway_is_valid);
        vm.addNativeMethod(g_pathwayClass, "setPath", pathway_set_path);
        vm.addNativeMethod(g_pathwayClass, "getRadius", pathway_get_radius);
        vm.addNativeMethod(g_pathwayClass, "setRadius", pathway_set_radius);
        vm.addNativeMethod(g_pathwayClass, "length", pathway_length);
        vm.addNativeMethod(g_pathwayClass, "isCyclic", pathway_is_cyclic);
        vm.addNativeMethod(g_pathwayClass, "getPointCount", pathway_point_count);
        vm.addNativeMethod(g_pathwayClass, "getPoint", pathway_get_point);
        vm.addNativeMethod(g_pathwayClass, "mapPoint", pathway_map_point);
        vm.addNativeMethod(g_pathwayClass, "mapDistanceToPoint", pathway_map_distance_to_point);
        vm.addNativeMethod(g_pathwayClass, "mapPointToDistance", pathway_map_point_to_distance);
    }
}
