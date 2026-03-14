#pragma once

#include "bindings.hpp"
#include "raymath.h"

#include <chipmunk/chipmunk.h>
namespace ChipmunkBindings
{
    extern NativeClassDef *g_cpSpaceClass;
    extern NativeClassDef *g_cpBodyClass;
    extern NativeClassDef *g_cpShapeClass;
    extern NativeClassDef *g_cpConstraintClass;
    extern NativeStructDef *g_vector2Def;

    void register_misc(ModuleBuilder &module, Interpreter &vm);
    void register_space(ModuleBuilder &module, Interpreter &vm);
    void register_body(ModuleBuilder &module, Interpreter &vm);
    void register_shape(ModuleBuilder &module, Interpreter &vm);
    void register_constraint(ModuleBuilder &module, Interpreter &vm);

    bool read_number_arg(const Value &value, double *out, const char *fn, int argIndex);
    bool read_int_arg(const Value &value, int *out, const char *fn, int argIndex);
    bool read_vector2_arg(const Value &value, Vector2 *out, const char *fn, int argIndex);
    bool read_cpvect_arg(const Value &value, cpVect *out, const char *fn, int argIndex);
    bool read_cpvect_array_arg(const Value &value, cpVect **outVerts, int *outCount, const char *fn, int argIndex);
    void free_cpvect_array(cpVect *verts);

    void push_vector2_components(Interpreter *vm, const Vector2 &value);
    void push_cpvect_components(Interpreter *vm, const cpVect &value);

    bool push_space_handle(Interpreter *vm, cpSpace *space);
    bool push_body_handle(Interpreter *vm, cpBody *body);
    bool push_shape_handle(Interpreter *vm, cpShape *shape);
    bool push_constraint_handle(Interpreter *vm, cpConstraint *constraint);

    cpSpace *require_space_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex);
    cpBody *require_body_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex);
    cpShape *require_shape_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex);
    cpConstraint *require_constraint_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex);

    void invalidate_native_handle(const Value &value);
}
