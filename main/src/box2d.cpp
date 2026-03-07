#include "bindings.hpp"
#include "utils.hpp"
#include <box2d/box2d.h>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL2/SDL_opengl.h>

namespace SDLBindings
{
    static constexpr const char *kClassB2World = "B2World";
    static constexpr const char *kClassB2Body = "B2Body";
    static constexpr const char *kClassB2BodyDef = "B2BodyDef";
    static constexpr const char *kClassB2PolygonShape = "B2PolygonShape";
    static constexpr const char *kClassB2CircleShape = "B2CircleShape";
    static constexpr const char *kClassB2EdgeShape = "B2EdgeShape";
    static constexpr const char *kClassB2ChainShape = "B2ChainShape";
    static constexpr const char *kClassB2FixtureDef = "B2FixtureDef";
    static constexpr const char *kClassB2DebugDraw = "B2DebugDraw";
    static constexpr const char *kClassB2ContactListener = "B2ContactListener";
    static constexpr const char *kClassB2QueryCallback = "B2QueryCallback";
    static constexpr const char *kClassB2RayCastCallback = "B2RayCastCallback";
    static constexpr const char *kStructB2Vec2 = "b2Vec2";
    static constexpr const char *kStructB2Vec3 = "b2Vec3";
    static constexpr const char *kStructB2Mat22 = "b2Mat22";
    static constexpr const char *kStructB2Mat33 = "b2Mat33";
    static constexpr const char *kStructB2Rot = "b2Rot";
    static constexpr const char *kStructB2Transform = "b2Transform";

    static NativeClassDef *g_b2BodyClass = nullptr;
    static NativeClassDef *g_b2DebugDrawClass = nullptr;
    static NativeClassDef *g_b2ContactListenerClass = nullptr;
    static NativeClassDef *g_b2QueryCallbackClass = nullptr;
    static NativeClassDef *g_b2RayCastCallbackClass = nullptr;
    static NativeStructDef *g_b2Vec2Def = nullptr;
    static NativeStructDef *g_b2Vec3Def = nullptr;
    static NativeStructDef *g_b2Mat22Def = nullptr;
    static NativeStructDef *g_b2Mat33Def = nullptr;
    static NativeStructDef *g_b2RotDef = nullptr;
    static NativeStructDef *g_b2TransformDef = nullptr;
    static std::unordered_map<b2Body *, std::string> g_b2BodyTags;

    static bool value_to_bool(const Value &v);
    static Value make_b2body_value(Interpreter *vm, b2Body *body);
    static void set_world_pin(Interpreter *vm, b2World *world, const char *tag, const Value &value);
    static void clear_world_pin(Interpreter *vm, b2World *world, const char *tag);

    struct B2DebugDrawNative : public b2Draw
    {
        Interpreter *vm = nullptr;
        Value scriptOwner = Value();
        bool scriptHookBusy = false;
        float lineWidth = 1.0f;
        float fillAlpha = 0.18f;
        float axisScale = 0.40f;
        float pointSize = 4.0f;

        B2DebugDrawNative() = default;
        virtual ~B2DebugDrawNative() = default;

        void bindScriptOwner(Interpreter *ownerVm, const Value &owner)
        {
            vm = ownerVm;
            scriptOwner = owner;
        }

        bool canCallScript() const
        {
            return vm && scriptOwner.isClassInstance();
        }

        bool callScript(const char *method, int argCount, Value *args) const
        {
            if (!canCallScript())
                return false;

            B2DebugDrawNative *self = const_cast<B2DebugDrawNative *>(this);
            if (self->scriptHookBusy)
                return false;

            const int topBefore = vm->getTop();
            self->scriptHookBusy = true;
            bool ok = vm->callMethod(scriptOwner, method, argCount, args);
            vm->setTop(topBefore);
            self->scriptHookBusy = false;
            return ok;
        }

        Value makeVertexArray(const b2Vec2 *vertices, int32 vertexCount) const
        {
            Value verts = vm->makeArray();
            ArrayInstance *arr = verts.asArray();
            if (!arr || vertexCount <= 0)
                return verts;
            arr->values.reserve((size_t)vertexCount * 2u);
            for (int32 i = 0; i < vertexCount; ++i)
            {
                arr->values.push(vm->makeDouble(vertices[i].x));
                arr->values.push(vm->makeDouble(vertices[i].y));
            }
            return verts;
        }

        void DrawPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) override
        {
            if (!canCallScript() || vertexCount <= 0)
                return;
            Value args[4];
            args[0] = makeVertexArray(vertices, vertexCount);
            args[1] = vm->makeDouble(color.r);
            args[2] = vm->makeDouble(color.g);
            args[3] = vm->makeDouble(color.b);
            (void)callScript("DrawPolygon", 4, args);
        }

        void DrawSolidPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) override
        {
            if (!canCallScript() || vertexCount <= 0)
                return;
            Value args[4];
            args[0] = makeVertexArray(vertices, vertexCount);
            args[1] = vm->makeDouble(color.r);
            args[2] = vm->makeDouble(color.g);
            args[3] = vm->makeDouble(color.b);
            (void)callScript("DrawSolidPolygon", 4, args);
        }

        void DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) override
        {
            if (!canCallScript())
                return;
            Value args[6];
            args[0] = vm->makeDouble(center.x);
            args[1] = vm->makeDouble(center.y);
            args[2] = vm->makeDouble(radius);
            args[3] = vm->makeDouble(color.r);
            args[4] = vm->makeDouble(color.g);
            args[5] = vm->makeDouble(color.b);
            (void)callScript("DrawCircle", 6, args);
        }

        void DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis, const b2Color &color) override
        {
            if (!canCallScript())
                return;
            Value args[8];
            args[0] = vm->makeDouble(center.x);
            args[1] = vm->makeDouble(center.y);
            args[2] = vm->makeDouble(radius);
            args[3] = vm->makeDouble(axis.x);
            args[4] = vm->makeDouble(axis.y);
            args[5] = vm->makeDouble(color.r);
            args[6] = vm->makeDouble(color.g);
            args[7] = vm->makeDouble(color.b);
            (void)callScript("DrawSolidCircle", 8, args);
        }

        void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) override
        {
            if (!canCallScript())
                return;
            Value args[7];
            args[0] = vm->makeDouble(p1.x);
            args[1] = vm->makeDouble(p1.y);
            args[2] = vm->makeDouble(p2.x);
            args[3] = vm->makeDouble(p2.y);
            args[4] = vm->makeDouble(color.r);
            args[5] = vm->makeDouble(color.g);
            args[6] = vm->makeDouble(color.b);
            (void)callScript("DrawSegment", 7, args);
        }

        void DrawTransform(const b2Transform &xf) override
        {
            if (!canCallScript())
                return;
            Value args[4];
            args[0] = vm->makeDouble(xf.p.x);
            args[1] = vm->makeDouble(xf.p.y);
            args[2] = vm->makeDouble(xf.q.s);
            args[3] = vm->makeDouble(xf.q.c);
            (void)callScript("DrawTransform", 4, args);
        }

        void DrawPoint(const b2Vec2 &p, float size, const b2Color &color) override
        {
            if (!canCallScript())
                return;
            Value args[6];
            args[0] = vm->makeDouble(p.x);
            args[1] = vm->makeDouble(p.y);
            args[2] = vm->makeDouble(size);
            args[3] = vm->makeDouble(color.r);
            args[4] = vm->makeDouble(color.g);
            args[5] = vm->makeDouble(color.b);
            (void)callScript("DrawPoint", 6, args);
        }
    };

    struct B2ContactListenerNative : public b2ContactListener
    {
        Interpreter *vm = nullptr;
        Value scriptOwner = Value();
        bool scriptHookBusy = false;

        void bindScriptOwner(Interpreter *ownerVm, const Value &owner)
        {
            vm = ownerVm;
            scriptOwner = owner;
        }

        bool canCallScript() const
        {
            return vm && scriptOwner.isClassInstance();
        }

        void callEvent(const char *method, b2Body *bodyA, b2Body *bodyB)
        {
            if (!canCallScript() || scriptHookBusy)
                return;

            Value args[2];
            args[0] = make_b2body_value(vm, bodyA);
            args[1] = make_b2body_value(vm, bodyB);

            const int topBefore = vm->getTop();
            scriptHookBusy = true;
            (void)vm->callMethod(scriptOwner, method, 2, args);
            vm->setTop(topBefore);
            scriptHookBusy = false;
        }

        void BeginContact(b2Contact *contact) override
        {
            if (!contact)
                return;
            b2Fixture *a = contact->GetFixtureA();
            b2Fixture *b = contact->GetFixtureB();
            callEvent("BeginContact", a ? a->GetBody() : nullptr, b ? b->GetBody() : nullptr);
        }

        void EndContact(b2Contact *contact) override
        {
            if (!contact)
                return;
            b2Fixture *a = contact->GetFixtureA();
            b2Fixture *b = contact->GetFixtureB();
            callEvent("EndContact", a ? a->GetBody() : nullptr, b ? b->GetBody() : nullptr);
        }

        void PreSolve(b2Contact *contact, const b2Manifold *oldManifold) override
        {
            (void)oldManifold;
            if (!contact)
                return;
            b2Fixture *a = contact->GetFixtureA();
            b2Fixture *b = contact->GetFixtureB();
            callEvent("PreSolve", a ? a->GetBody() : nullptr, b ? b->GetBody() : nullptr);
        }

        void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) override
        {
            (void)impulse;
            if (!contact)
                return;
            b2Fixture *a = contact->GetFixtureA();
            b2Fixture *b = contact->GetFixtureB();
            callEvent("PostSolve", a ? a->GetBody() : nullptr, b ? b->GetBody() : nullptr);
        }
    };

    struct B2QueryCallbackNative : public b2QueryCallback
    {
        Interpreter *vm = nullptr;
        Value scriptOwner = Value();
        bool scriptHookBusy = false;
        int hitCount = 0;

        void bindScriptOwner(Interpreter *ownerVm, const Value &owner)
        {
            vm = ownerVm;
            scriptOwner = owner;
        }

        bool canCallScript() const
        {
            return vm && scriptOwner.isClassInstance();
        }

        bool ReportFixture(b2Fixture *fixture) override
        {
            hitCount++;
            if (!fixture || !canCallScript() || scriptHookBusy)
                return true;

            Value args[1];
            args[0] = make_b2body_value(vm, fixture->GetBody());

            const int topBefore = vm->getTop();
            scriptHookBusy = true;
            bool ok = vm->callMethod(scriptOwner, "ReportFixture", 1, args);
            bool keepGoing = true;
            if (ok && vm->getTop() > topBefore)
            {
                keepGoing = value_to_bool(vm->peek(-1));
            }
            vm->setTop(topBefore);
            scriptHookBusy = false;
            return keepGoing;
        }
    };

    struct B2RayCastCallbackNative : public b2RayCastCallback
    {
        Interpreter *vm = nullptr;
        Value scriptOwner = Value();
        bool scriptHookBusy = false;
        int hitCount = 0;

        void bindScriptOwner(Interpreter *ownerVm, const Value &owner)
        {
            vm = ownerVm;
            scriptOwner = owner;
        }

        bool canCallScript() const
        {
            return vm && scriptOwner.isClassInstance();
        }

        float ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal, float fraction) override
        {
            hitCount++;
            if (!fixture || !canCallScript() || scriptHookBusy)
                return 1.0f;

            Value args[6];
            args[0] = make_b2body_value(vm, fixture->GetBody());
            args[1] = vm->makeDouble(point.x);
            args[2] = vm->makeDouble(point.y);
            args[3] = vm->makeDouble(normal.x);
            args[4] = vm->makeDouble(normal.y);
            args[5] = vm->makeDouble(fraction);

            const int topBefore = vm->getTop();
            scriptHookBusy = true;
            bool ok = vm->callMethod(scriptOwner, "ReportFixture", 6, args);
            float out = 1.0f;
            if (ok && vm->getTop() > topBefore)
            {
                const Value &ret = vm->peek(-1);
                if (ret.isNumber())
                {
                    double v = ret.asNumber();
                    out = (float)(std::isfinite(v) ? v : 1.0);
                }
                else if (ret.isBool())
                {
                    out = ret.asBool() ? 1.0f : 0.0f;
                }
                else if (ret.isNil())
                {
                    out = 1.0f;
                }
            }
            vm->setTop(topBefore);
            scriptHookBusy = false;
            return out;
        }
    };

    static int push_nil1(Interpreter *vm)
    {
        vm->pushNil();
        return 1;
    }

    static int push_nil2(Interpreter *vm)
    {
        vm->pushNil();
        vm->pushNil();
        return 2;
    }

    static b2World *as_world(void *instance) { return (b2World *)instance; }
    static b2Body *as_body(void *instance) { return (b2Body *)instance; }
    static b2BodyDef *as_bodydef(void *instance) { return (b2BodyDef *)instance; }
    static b2PolygonShape *as_polygon_shape(void *instance) { return (b2PolygonShape *)instance; }
    static b2CircleShape *as_circle_shape(void *instance) { return (b2CircleShape *)instance; }
    static b2EdgeShape *as_edge_shape(void *instance) { return (b2EdgeShape *)instance; }
    static b2ChainShape *as_chain_shape(void *instance) { return (b2ChainShape *)instance; }
    static b2FixtureDef *as_fixturedef(void *instance) { return (b2FixtureDef *)instance; }
    static B2DebugDrawNative *as_debugdraw(void *instance) { return (B2DebugDrawNative *)instance; }
    static B2ContactListenerNative *as_contact_listener(void *instance) { return (B2ContactListenerNative *)instance; }
    static B2QueryCallbackNative *as_query_callback(void *instance) { return (B2QueryCallbackNative *)instance; }
    static B2RayCastCallbackNative *as_raycast_callback(void *instance) { return (B2RayCastCallbackNative *)instance; }

    static bool value_to_bool(const Value &v)
    {
        if (v.isBool())
            return v.asBool();
        if (v.isNumber())
            return v.asNumber() != 0.0;
        if (v.isNil())
            return false;
        return true;
    }

    static Value make_b2body_value(Interpreter *vm, b2Body *body)
    {
        if (!vm || !g_b2BodyClass || !body)
            return vm ? vm->makeNil() : Value();

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst)
            return vm->makeNil();

        inst->klass = g_b2BodyClass;
        inst->userData = body;
        return value;
    }

    static void set_world_pin(Interpreter *vm, b2World *world, const char *tag, const Value &value)
    {
        if (!vm || !world || !tag)
            return;

        char key[128];
        std::snprintf(key, sizeof(key), "__b2_pin_%s_%p", tag, (void *)world);
        if (!vm->setGlobal(key, value))
        {
            vm->addGlobal(key, value);
        }
    }

    static void clear_world_pin(Interpreter *vm, b2World *world, const char *tag)
    {
        if (!vm || !world || !tag)
            return;
        set_world_pin(vm, world, tag, vm->makeNil());
    }

    static bool get_world_from_instance(void *instance, b2World **outWorld, const char *fnName)
    {
        b2World *world = as_world(instance);
        if (!world)
        {
            Error("%s on invalid B2World", fnName);
            return false;
        }
        *outWorld = world;
        return true;
    }

    static bool get_body_from_instance(void *instance, b2Body **outBody, const char *fnName)
    {
        b2Body *body = as_body(instance);
        if (!body)
        {
            Error("%s on invalid B2Body", fnName);
            return false;
        }
        *outBody = body;
        return true;
    }

    static bool try_get_native_value_ptr(const Value &value, const char *className, void **outPtr)
    {
        if (value.isNativeClassInstance())
        {
            NativeClassInstance *inst = value.asNativeClassInstance();
            if (!inst || !inst->klass || !inst->klass->name || !inst->userData)
                return false;

            const char *actual = inst->klass->name->chars();
            if (!actual || std::strcmp(actual, className) != 0)
                return false;

            *outPtr = inst->userData;
            return true;
        }

        // Script class that inherits from native class.
        if (value.isClassInstance())
        {
            ClassInstance *inst = value.asClassInstance();
            if (!inst || !inst->klass || !inst->nativeUserData)
                return false;

            NativeClassDef *nativeSuper = inst->getNativeSuperclass();
            if (!nativeSuper || !nativeSuper->name)
                return false;

            const char *actual = nativeSuper->name->chars();
            if (!actual || std::strcmp(actual, className) != 0)
                return false;

            *outPtr = inst->nativeUserData;
            return true;
        }

        return false;
    }

    static bool get_native_value_ptr(const Value &value, const char *className, void **outPtr, const char *fnName)
    {
        if (!try_get_native_value_ptr(value, className, outPtr))
        {
            Error("%s expects %s", fnName, className);
            return false;
        }
        return true;
    }

    static bool push_b2body_instance(Interpreter *vm, b2Body *body)
    {
        if (!vm)
            return false;
        Value value = make_b2body_value(vm, body);
        vm->push(value);
        return !value.isNil();
    }

    // ---------------- b2Math structs ----------------

    static bool read_number(const Value &v, double *out, const char *fn, int argIndex)
    {
        if (!v.isNumber())
        {
            Error("%s arg %d expects number", fn, argIndex);
            return false;
        }
        *out = v.asNumber();
        return true;
    }

    static bool read_b2vec2(const Value &v, const char *fn, int argIndex, b2Vec2 *out);

    static bool read_vec2_array(const Value &v, std::vector<b2Vec2> *outVerts, const char *fn, int argIndex, int minVerts)
    {
        if (!v.isArray())
        {
            Error("%s arg %d expects array", fn, argIndex);
            return false;
        }

        ArrayInstance *arr = v.asArray();
        if (!arr)
        {
            Error("%s arg %d invalid array", fn, argIndex);
            return false;
        }

        const int n = (int)arr->values.size();
        outVerts->clear();
        if (n <= 0)
        {
            Error("%s arg %d array is empty", fn, argIndex);
            return false;
        }

        // Variant A: array of b2Vec2 structs
        if (arr->values[0].isNativeStructInstance())
        {
            outVerts->reserve((size_t)n);
            for (int i = 0; i < n; i++)
            {
                b2Vec2 p;
                if (!read_b2vec2(arr->values[i], fn, argIndex, &p))
                    return false;
                outVerts->push_back(p);
            }
        }
        else
        {
            // Variant B: flat numeric array [x0, y0, x1, y1, ...]
            if ((n % 2) != 0)
            {
                Error("%s arg %d flat numeric array must have even length", fn, argIndex);
                return false;
            }
            outVerts->reserve((size_t)(n / 2));
            for (int i = 0; i < n; i += 2)
            {
                if (!arr->values[i].isNumber() || !arr->values[i + 1].isNumber())
                {
                    Error("%s arg %d flat numeric array must contain only numbers", fn, argIndex);
                    return false;
                }
                outVerts->push_back(b2Vec2((float)arr->values[i].asNumber(), (float)arr->values[i + 1].asNumber()));
            }
        }

        if ((int)outVerts->size() < minVerts)
        {
            Error("%s arg %d expects at least %d vertices", fn, argIndex, minVerts);
            return false;
        }
        return true;
    }

    static bool read_struct_data(const Value &v, NativeStructDef *def, const char *typeName, const char *fn, int argIndex, void **outData)
    {
        if (!v.isNativeStructInstance())
        {
            Error("%s arg %d expects %s", fn, argIndex, typeName);
            return false;
        }

        NativeStructInstance *inst = v.asNativeStructInstance();
        if (!inst || inst->def != def || !inst->data)
        {
            Error("%s arg %d expects %s", fn, argIndex, typeName);
            return false;
        }

        *outData = inst->data;
        return true;
    }

    static bool try_read_b2vec2(const Value &v, b2Vec2 *out)
    {
        if (!v.isNativeStructInstance() || !g_b2Vec2Def)
            return false;
        NativeStructInstance *inst = v.asNativeStructInstance();
        if (!inst || inst->def != g_b2Vec2Def || !inst->data)
            return false;
        *out = *(b2Vec2 *)inst->data;
        return true;
    }

    static bool try_read_b2vec3(const Value &v, b2Vec3 *out)
    {
        if (!v.isNativeStructInstance() || !g_b2Vec3Def)
            return false;
        NativeStructInstance *inst = v.asNativeStructInstance();
        if (!inst || inst->def != g_b2Vec3Def || !inst->data)
            return false;
        *out = *(b2Vec3 *)inst->data;
        return true;
    }

    static bool try_read_b2rot(const Value &v, b2Rot *out)
    {
        if (!v.isNativeStructInstance() || !g_b2RotDef)
            return false;
        NativeStructInstance *inst = v.asNativeStructInstance();
        if (!inst || inst->def != g_b2RotDef || !inst->data)
            return false;
        *out = *(b2Rot *)inst->data;
        return true;
    }

    static bool read_b2vec2_ptr(const Value &v, const char *fn, int argIndex, b2Vec2 **out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2Vec2Def, kStructB2Vec2, fn, argIndex, &data))
            return false;
        *out = (b2Vec2 *)data;
        return true;
    }

    static bool read_b2vec2(const Value &v, const char *fn, int argIndex, b2Vec2 *out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2Vec2Def, kStructB2Vec2, fn, argIndex, &data))
            return false;
        *out = *(b2Vec2 *)data;
        return true;
    }

    static bool read_b2vec3_ptr(const Value &v, const char *fn, int argIndex, b2Vec3 **out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2Vec3Def, kStructB2Vec3, fn, argIndex, &data))
            return false;
        *out = (b2Vec3 *)data;
        return true;
    }

    static bool read_b2vec3(const Value &v, const char *fn, int argIndex, b2Vec3 *out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2Vec3Def, kStructB2Vec3, fn, argIndex, &data))
            return false;
        *out = *(b2Vec3 *)data;
        return true;
    }

    static bool read_b2mat22_ptr(const Value &v, const char *fn, int argIndex, b2Mat22 **out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2Mat22Def, kStructB2Mat22, fn, argIndex, &data))
            return false;
        *out = (b2Mat22 *)data;
        return true;
    }

    static bool read_b2mat22(const Value &v, const char *fn, int argIndex, b2Mat22 *out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2Mat22Def, kStructB2Mat22, fn, argIndex, &data))
            return false;
        *out = *(b2Mat22 *)data;
        return true;
    }

    static bool read_b2mat33_ptr(const Value &v, const char *fn, int argIndex, b2Mat33 **out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2Mat33Def, kStructB2Mat33, fn, argIndex, &data))
            return false;
        *out = (b2Mat33 *)data;
        return true;
    }

    static bool read_b2mat33(const Value &v, const char *fn, int argIndex, b2Mat33 *out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2Mat33Def, kStructB2Mat33, fn, argIndex, &data))
            return false;
        *out = *(b2Mat33 *)data;
        return true;
    }

    static bool read_b2rot_ptr(const Value &v, const char *fn, int argIndex, b2Rot **out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2RotDef, kStructB2Rot, fn, argIndex, &data))
            return false;
        *out = (b2Rot *)data;
        return true;
    }

    static bool read_b2rot(const Value &v, const char *fn, int argIndex, b2Rot *out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2RotDef, kStructB2Rot, fn, argIndex, &data))
            return false;
        *out = *(b2Rot *)data;
        return true;
    }

    static bool read_b2transform_ptr(const Value &v, const char *fn, int argIndex, b2Transform **out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2TransformDef, kStructB2Transform, fn, argIndex, &data))
            return false;
        *out = (b2Transform *)data;
        return true;
    }

    static bool read_b2transform(const Value &v, const char *fn, int argIndex, b2Transform *out)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_b2TransformDef, kStructB2Transform, fn, argIndex, &data))
            return false;
        *out = *(b2Transform *)data;
        return true;
    }

    static bool push_b2vec2(Interpreter *vm, const b2Vec2 &v)
    {
        if (!g_b2Vec2Def)
            return false;
        Value out = vm->createNativeStruct(g_b2Vec2Def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(b2Vec2 *)inst->data = v;
        vm->push(out);
        return true;
    }

    static bool push_b2vec3(Interpreter *vm, const b2Vec3 &v)
    {
        if (!g_b2Vec3Def)
            return false;
        Value out = vm->createNativeStruct(g_b2Vec3Def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(b2Vec3 *)inst->data = v;
        vm->push(out);
        return true;
    }

    static bool push_b2mat22(Interpreter *vm, const b2Mat22 &m)
    {
        if (!g_b2Mat22Def)
            return false;
        Value out = vm->createNativeStruct(g_b2Mat22Def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(b2Mat22 *)inst->data = m;
        vm->push(out);
        return true;
    }

    static bool push_b2mat33(Interpreter *vm, const b2Mat33 &m)
    {
        if (!g_b2Mat33Def)
            return false;
        Value out = vm->createNativeStruct(g_b2Mat33Def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(b2Mat33 *)inst->data = m;
        vm->push(out);
        return true;
    }

    static bool push_b2rot(Interpreter *vm, const b2Rot &r)
    {
        if (!g_b2RotDef)
            return false;
        Value out = vm->createNativeStruct(g_b2RotDef->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(b2Rot *)inst->data = r;
        vm->push(out);
        return true;
    }

    static bool push_b2transform(Interpreter *vm, const b2Transform &t)
    {
        if (!g_b2TransformDef)
            return false;
        Value out = vm->createNativeStruct(g_b2TransformDef->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(b2Transform *)inst->data = t;
        vm->push(out);
        return true;
    }

    static void b2vec2_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        b2Vec2 *v = (b2Vec2 *)buffer;
        v->SetZero();
        if (argc > 0 && args[0].isNumber())
            v->x = (float)args[0].asNumber();
        if (argc > 1 && args[1].isNumber())
            v->y = (float)args[1].asNumber();
    }

    static void b2vec3_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        b2Vec3 *v = (b2Vec3 *)buffer;
        v->SetZero();
        if (argc > 0 && args[0].isNumber())
            v->x = (float)args[0].asNumber();
        if (argc > 1 && args[1].isNumber())
            v->y = (float)args[1].asNumber();
        if (argc > 2 && args[2].isNumber())
            v->z = (float)args[2].asNumber();
    }

    static void b2mat22_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        b2Mat22 *m = (b2Mat22 *)buffer;
        m->SetZero();

        if (argc == 0)
            return;

        if (argc == 4 && args[0].isNumber() && args[1].isNumber() && args[2].isNumber() && args[3].isNumber())
        {
            *m = b2Mat22((float)args[0].asNumber(), (float)args[1].asNumber(), (float)args[2].asNumber(), (float)args[3].asNumber());
            return;
        }

        b2Vec2 c1, c2;
        if (argc == 2 && try_read_b2vec2(args[0], &c1) && try_read_b2vec2(args[1], &c2))
        {
            m->Set(c1, c2);
            return;
        }

        Error("b2Mat22 constructor expects (), (a11,a12,a21,a22) or (b2Vec2,b2Vec2)");
    }

    static void b2mat33_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        b2Mat33 *m = (b2Mat33 *)buffer;
        m->SetZero();

        if (argc == 0)
            return;

        if (argc == 9)
        {
            for (int i = 0; i < 9; i++)
            {
                if (!args[i].isNumber())
                {
                    Error("b2Mat33 constructor with 9 args expects all numbers");
                    return;
                }
            }
            m->ex.Set((float)args[0].asNumber(), (float)args[1].asNumber(), (float)args[2].asNumber());
            m->ey.Set((float)args[3].asNumber(), (float)args[4].asNumber(), (float)args[5].asNumber());
            m->ez.Set((float)args[6].asNumber(), (float)args[7].asNumber(), (float)args[8].asNumber());
            return;
        }

        b2Vec3 c1, c2, c3;
        if (argc == 3 && try_read_b2vec3(args[0], &c1) && try_read_b2vec3(args[1], &c2) && try_read_b2vec3(args[2], &c3))
        {
            *m = b2Mat33(c1, c2, c3);
            return;
        }

        Error("b2Mat33 constructor expects (), (9 numbers) or (b2Vec3,b2Vec3,b2Vec3)");
    }

    static void b2rot_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        b2Rot *r = (b2Rot *)buffer;
        r->SetIdentity();

        if (argc == 0)
            return;
        if (argc == 1 && args[0].isNumber())
        {
            r->Set((float)args[0].asNumber());
            return;
        }
        Error("b2Rot constructor expects () or (angleRadians)");
    }

    static void b2transform_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        b2Transform *t = (b2Transform *)buffer;
        t->SetIdentity();

        if (argc == 0)
            return;

        if (argc == 3 && args[0].isNumber() && args[1].isNumber() && args[2].isNumber())
        {
            t->Set(b2Vec2((float)args[0].asNumber(), (float)args[1].asNumber()), (float)args[2].asNumber());
            return;
        }

        b2Vec2 p;
        b2Rot r;
        if (argc == 2 && try_read_b2vec2(args[0], &p) && try_read_b2rot(args[1], &r))
        {
            t->p = p;
            t->q = r;
            return;
        }
        if (argc == 2 && try_read_b2vec2(args[0], &p) && args[1].isNumber())
        {
            t->Set(p, (float)args[1].asNumber());
            return;
        }

        Error("b2Transform constructor expects (), (x,y,angle), (b2Vec2,b2Rot) or (b2Vec2,angle)");
    }

    // ---------------- b2Math functions ----------------

    static int b2math_vec2_set_zero(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("b2Vec2_SetZero expects (b2Vec2)");
            return 0;
        }
        b2Vec2 *v = nullptr;
        if (!read_b2vec2_ptr(args[0], "b2Vec2_SetZero", 1, &v))
            return 0;
        v->SetZero();
        return 0;
    }

    static int b2math_vec2_set(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("b2Vec2_Set expects (b2Vec2, x, y)");
            return 0;
        }
        b2Vec2 *v = nullptr;
        double x = 0.0, y = 0.0;
        if (!read_b2vec2_ptr(args[0], "b2Vec2_Set", 1, &v) ||
            !read_number(args[1], &x, "b2Vec2_Set", 2) ||
            !read_number(args[2], &y, "b2Vec2_Set", 3))
            return 0;
        v->Set((float)x, (float)y);
        return 0;
    }

    static int b2math_vec2_length(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Vec2_Length expects (b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 v;
        if (!read_b2vec2(args[0], "b2Vec2_Length", 1, &v))
            return push_nil1(vm);
        vm->pushDouble(v.Length());
        return 1;
    }

    static int b2math_vec2_length_squared(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Vec2_LengthSquared expects (b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 v;
        if (!read_b2vec2(args[0], "b2Vec2_LengthSquared", 1, &v))
            return push_nil1(vm);
        vm->pushDouble(v.LengthSquared());
        return 1;
    }

    static int b2math_vec2_normalize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Vec2_Normalize expects (b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 *v = nullptr;
        if (!read_b2vec2_ptr(args[0], "b2Vec2_Normalize", 1, &v))
            return push_nil1(vm);
        vm->pushDouble(v->Normalize());
        return 1;
    }

    static int b2math_vec2_is_valid(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Vec2_IsValid expects (b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 v;
        if (!read_b2vec2(args[0], "b2Vec2_IsValid", 1, &v))
            return push_nil1(vm);
        vm->pushBool(v.IsValid());
        return 1;
    }

    static int b2math_vec2_skew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Vec2_Skew expects (b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 v;
        if (!read_b2vec2(args[0], "b2Vec2_Skew", 1, &v))
            return push_nil1(vm);
        if (!push_b2vec2(vm, v.Skew()))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_vec3_set_zero(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("b2Vec3_SetZero expects (b2Vec3)");
            return 0;
        }
        b2Vec3 *v = nullptr;
        if (!read_b2vec3_ptr(args[0], "b2Vec3_SetZero", 1, &v))
            return 0;
        v->SetZero();
        return 0;
    }

    static int b2math_vec3_set(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4)
        {
            Error("b2Vec3_Set expects (b2Vec3, x, y, z)");
            return 0;
        }
        b2Vec3 *v = nullptr;
        double x = 0.0, y = 0.0, z = 0.0;
        if (!read_b2vec3_ptr(args[0], "b2Vec3_Set", 1, &v) ||
            !read_number(args[1], &x, "b2Vec3_Set", 2) ||
            !read_number(args[2], &y, "b2Vec3_Set", 3) ||
            !read_number(args[3], &z, "b2Vec3_Set", 4))
            return 0;
        v->Set((float)x, (float)y, (float)z);
        return 0;
    }

    static int b2math_mat22_set_identity(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("b2Mat22_SetIdentity expects (b2Mat22)");
            return 0;
        }
        b2Mat22 *m = nullptr;
        if (!read_b2mat22_ptr(args[0], "b2Mat22_SetIdentity", 1, &m))
            return 0;
        m->SetIdentity();
        return 0;
    }

    static int b2math_mat22_set_zero(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("b2Mat22_SetZero expects (b2Mat22)");
            return 0;
        }
        b2Mat22 *m = nullptr;
        if (!read_b2mat22_ptr(args[0], "b2Mat22_SetZero", 1, &m))
            return 0;
        m->SetZero();
        return 0;
    }

    static int b2math_mat22_get_inverse(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Mat22_GetInverse expects (b2Mat22)");
            return push_nil1(vm);
        }
        b2Mat22 m;
        if (!read_b2mat22(args[0], "b2Mat22_GetInverse", 1, &m))
            return push_nil1(vm);
        if (!push_b2mat22(vm, m.GetInverse()))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mat22_solve(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Mat22_Solve expects (b2Mat22, b2Vec2)");
            return push_nil1(vm);
        }
        b2Mat22 m;
        b2Vec2 b;
        if (!read_b2mat22(args[0], "b2Mat22_Solve", 1, &m) ||
            !read_b2vec2(args[1], "b2Mat22_Solve", 2, &b))
            return push_nil1(vm);
        if (!push_b2vec2(vm, m.Solve(b)))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mat33_set_zero(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("b2Mat33_SetZero expects (b2Mat33)");
            return 0;
        }
        b2Mat33 *m = nullptr;
        if (!read_b2mat33_ptr(args[0], "b2Mat33_SetZero", 1, &m))
            return 0;
        m->SetZero();
        return 0;
    }

    static int b2math_mat33_solve22(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Mat33_Solve22 expects (b2Mat33, b2Vec2)");
            return push_nil1(vm);
        }
        b2Mat33 m;
        b2Vec2 b;
        if (!read_b2mat33(args[0], "b2Mat33_Solve22", 1, &m) ||
            !read_b2vec2(args[1], "b2Mat33_Solve22", 2, &b))
            return push_nil1(vm);
        if (!push_b2vec2(vm, m.Solve22(b)))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mat33_solve33(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Mat33_Solve33 expects (b2Mat33, b2Vec3)");
            return push_nil1(vm);
        }
        b2Mat33 m;
        b2Vec3 b;
        if (!read_b2mat33(args[0], "b2Mat33_Solve33", 1, &m) ||
            !read_b2vec3(args[1], "b2Mat33_Solve33", 2, &b))
            return push_nil1(vm);
        if (!push_b2vec3(vm, m.Solve33(b)))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mat33_get_inverse22(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Mat33_GetInverse22 expects (b2Mat33)");
            return push_nil1(vm);
        }
        b2Mat33 m, out;
        if (!read_b2mat33(args[0], "b2Mat33_GetInverse22", 1, &m))
            return push_nil1(vm);
        m.GetInverse22(&out);
        if (!push_b2mat33(vm, out))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mat33_get_sym_inverse33(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Mat33_GetSymInverse33 expects (b2Mat33)");
            return push_nil1(vm);
        }
        b2Mat33 m, out;
        if (!read_b2mat33(args[0], "b2Mat33_GetSymInverse33", 1, &m))
            return push_nil1(vm);
        m.GetSymInverse33(&out);
        if (!push_b2mat33(vm, out))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_rot_set(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("b2Rot_Set expects (b2Rot, angleRadians)");
            return 0;
        }
        b2Rot *r = nullptr;
        double angle = 0.0;
        if (!read_b2rot_ptr(args[0], "b2Rot_Set", 1, &r) ||
            !read_number(args[1], &angle, "b2Rot_Set", 2))
            return 0;
        r->Set((float)angle);
        return 0;
    }

    static int b2math_rot_set_identity(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("b2Rot_SetIdentity expects (b2Rot)");
            return 0;
        }
        b2Rot *r = nullptr;
        if (!read_b2rot_ptr(args[0], "b2Rot_SetIdentity", 1, &r))
            return 0;
        r->SetIdentity();
        return 0;
    }

    static int b2math_rot_get_angle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Rot_GetAngle expects (b2Rot)");
            return push_nil1(vm);
        }
        b2Rot r;
        if (!read_b2rot(args[0], "b2Rot_GetAngle", 1, &r))
            return push_nil1(vm);
        vm->pushDouble(r.GetAngle());
        return 1;
    }

    static int b2math_rot_get_x_axis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Rot_GetXAxis expects (b2Rot)");
            return push_nil1(vm);
        }
        b2Rot r;
        if (!read_b2rot(args[0], "b2Rot_GetXAxis", 1, &r))
            return push_nil1(vm);
        if (!push_b2vec2(vm, r.GetXAxis()))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_rot_get_y_axis(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Rot_GetYAxis expects (b2Rot)");
            return push_nil1(vm);
        }
        b2Rot r;
        if (!read_b2rot(args[0], "b2Rot_GetYAxis", 1, &r))
            return push_nil1(vm);
        if (!push_b2vec2(vm, r.GetYAxis()))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_transform_set_identity(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("b2Transform_SetIdentity expects (b2Transform)");
            return 0;
        }
        b2Transform *t = nullptr;
        if (!read_b2transform_ptr(args[0], "b2Transform_SetIdentity", 1, &t))
            return 0;
        t->SetIdentity();
        return 0;
    }

    static int b2math_transform_set(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3)
        {
            Error("b2Transform_Set expects (b2Transform, b2Vec2, angleRadians)");
            return 0;
        }
        b2Transform *t = nullptr;
        b2Vec2 p;
        double angle = 0.0;
        if (!read_b2transform_ptr(args[0], "b2Transform_Set", 1, &t) ||
            !read_b2vec2(args[1], "b2Transform_Set", 2, &p) ||
            !read_number(args[2], &angle, "b2Transform_Set", 3))
            return 0;
        t->Set(p, (float)angle);
        return 0;
    }

    static int b2math_transform_get_position(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Transform_GetPosition expects (b2Transform)");
            return push_nil1(vm);
        }
        b2Transform t;
        if (!read_b2transform(args[0], "b2Transform_GetPosition", 1, &t))
            return push_nil1(vm);
        if (!push_b2vec2(vm, t.p))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_transform_get_rotation(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("b2Transform_GetRotation expects (b2Transform)");
            return push_nil1(vm);
        }
        b2Transform t;
        if (!read_b2transform(args[0], "b2Transform_GetRotation", 1, &t))
            return push_nil1(vm);
        if (!push_b2rot(vm, t.q))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_dot(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Dot expects (b2Vec2, b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 a, b;
        if (!read_b2vec2(args[0], "b2Dot", 1, &a) || !read_b2vec2(args[1], "b2Dot", 2, &b))
            return push_nil1(vm);
        vm->pushDouble(b2Dot(a, b));
        return 1;
    }

    static int b2math_cross(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Cross expects (b2Vec2, b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 a, b;
        if (!read_b2vec2(args[0], "b2Cross", 1, &a) || !read_b2vec2(args[1], "b2Cross", 2, &b))
            return push_nil1(vm);
        vm->pushDouble(b2Cross(a, b));
        return 1;
    }

    static int b2math_distance(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Distance expects (b2Vec2, b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 a, b;
        if (!read_b2vec2(args[0], "b2Distance", 1, &a) || !read_b2vec2(args[1], "b2Distance", 2, &b))
            return push_nil1(vm);
        vm->pushDouble(b2Distance(a, b));
        return 1;
    }

    static int b2math_distance_squared(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2DistanceSquared expects (b2Vec2, b2Vec2)");
            return push_nil1(vm);
        }
        b2Vec2 a, b;
        if (!read_b2vec2(args[0], "b2DistanceSquared", 1, &a) || !read_b2vec2(args[1], "b2DistanceSquared", 2, &b))
            return push_nil1(vm);
        vm->pushDouble(b2DistanceSquared(a, b));
        return 1;
    }

    static int b2math_mul_rot_vec2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Mul_RotVec2 expects (b2Rot, b2Vec2)");
            return push_nil1(vm);
        }
        b2Rot q;
        b2Vec2 v;
        if (!read_b2rot(args[0], "b2Mul_RotVec2", 1, &q) || !read_b2vec2(args[1], "b2Mul_RotVec2", 2, &v))
            return push_nil1(vm);
        if (!push_b2vec2(vm, b2Mul(q, v)))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mul_t_rot_vec2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2MulT_RotVec2 expects (b2Rot, b2Vec2)");
            return push_nil1(vm);
        }
        b2Rot q;
        b2Vec2 v;
        if (!read_b2rot(args[0], "b2MulT_RotVec2", 1, &q) || !read_b2vec2(args[1], "b2MulT_RotVec2", 2, &v))
            return push_nil1(vm);
        if (!push_b2vec2(vm, b2MulT(q, v)))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mul_transform_vec2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Mul_TransformVec2 expects (b2Transform, b2Vec2)");
            return push_nil1(vm);
        }
        b2Transform t;
        b2Vec2 v;
        if (!read_b2transform(args[0], "b2Mul_TransformVec2", 1, &t) || !read_b2vec2(args[1], "b2Mul_TransformVec2", 2, &v))
            return push_nil1(vm);
        if (!push_b2vec2(vm, b2Mul(t, v)))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mul_t_transform_vec2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2MulT_TransformVec2 expects (b2Transform, b2Vec2)");
            return push_nil1(vm);
        }
        b2Transform t;
        b2Vec2 v;
        if (!read_b2transform(args[0], "b2MulT_TransformVec2", 1, &t) || !read_b2vec2(args[1], "b2MulT_TransformVec2", 2, &v))
            return push_nil1(vm);
        if (!push_b2vec2(vm, b2MulT(t, v)))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mul_transform_transform(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2Mul_TransformTransform expects (b2Transform, b2Transform)");
            return push_nil1(vm);
        }
        b2Transform a, b;
        if (!read_b2transform(args[0], "b2Mul_TransformTransform", 1, &a) ||
            !read_b2transform(args[1], "b2Mul_TransformTransform", 2, &b))
            return push_nil1(vm);
        if (!push_b2transform(vm, b2Mul(a, b)))
            return push_nil1(vm);
        return 1;
    }

    static int b2math_mul_t_transform_transform(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("b2MulT_TransformTransform expects (b2Transform, b2Transform)");
            return push_nil1(vm);
        }
        b2Transform a, b;
        if (!read_b2transform(args[0], "b2MulT_TransformTransform", 1, &a) ||
            !read_b2transform(args[1], "b2MulT_TransformTransform", 2, &b))
            return push_nil1(vm);
        if (!push_b2transform(vm, b2MulT(a, b)))
            return push_nil1(vm);
        return 1;
    }

    // ---------------- B2World ----------------

    static void *b2world_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("B2World expects (gravityX, gravityY)");
            return nullptr;
        }
        return new b2World(b2Vec2((float)args[0].asNumber(), (float)args[1].asNumber()));
    }

    static void b2world_dtor(Interpreter *vm, void *instance)
    {
        // Ownership is explicit via destroy(), Ogre-style.
        (void)vm;
        (void)instance;
    }

    static int b2world_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2World.destroy() expects 0 arguments");
            return push_nil1(vm);
        }

        b2World *world = as_world(instance);
        if (!world)
        {
            return push_nil1(vm);
        }

        clear_world_pin(vm, world, "debugdraw");
        clear_world_pin(vm, world, "contact");

        // Clear body tags owned by this world before world destruction.
        for (b2Body *b = world->GetBodyList(); b; b = b->GetNext())
        {
            g_b2BodyTags.erase(b);
        }
        delete world;
        vm->pushBool(true);
        return 1;
    }

    static int b2world_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2World.isValid() expects 0 arguments");
            return push_nil1(vm);
        }
        vm->pushBool(as_world(instance) != nullptr);
        return 1;
    }

    static int b2world_step(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1 || argCount > 3 || !args[0].isNumber())
        {
            Error("B2World.step() expects (dt[, velocityIterations, positionIterations])");
            return 0;
        }

        b2World *world = nullptr;
        if (!get_world_from_instance(instance, &world, "B2World.step()"))
            return 0;

        const int velocityIterations = (argCount >= 2) ? args[1].asInt() : 8;
        const int positionIterations = (argCount >= 3) ? args[2].asInt() : 3;
        world->Step((float)args[0].asNumber(), velocityIterations, positionIterations);
        return 0;
    }

    static int b2world_create_body(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2World.createBody() expects (bodyDef)");
            return push_nil1(vm);
        }

        b2World *world = nullptr;
        if (!get_world_from_instance(instance, &world, "B2World.createBody()"))
            return push_nil1(vm);

        void *ptr = nullptr;
        if (!get_native_value_ptr(args[0], kClassB2BodyDef, &ptr, "B2World.createBody()"))
            return push_nil1(vm);
        b2BodyDef *def = (b2BodyDef *)ptr;

        b2Body *body = world->CreateBody(def);
        if (!body)
        {
            Error("B2World.createBody() failed");
            return push_nil1(vm);
        }

        if (!push_b2body_instance(vm, body))
            return 1;
        return 1;
    }

    static int b2world_set_debug_draw(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2World.setDebugDraw() expects (debugDrawOrNil)");
            return push_nil1(vm);
        }

        b2World *world = nullptr;
        if (!get_world_from_instance(instance, &world, "B2World.setDebugDraw()"))
            return push_nil1(vm);

        if (args[0].isNil())
        {
            world->SetDebugDraw(nullptr);
            clear_world_pin(vm, world, "debugdraw");
            vm->pushBool(true);
            return 1;
        }

        void *ptr = nullptr;
        if (!get_native_value_ptr(args[0], kClassB2DebugDraw, &ptr, "B2World.setDebugDraw()"))
            return push_nil1(vm);

        B2DebugDrawNative *dd = (B2DebugDrawNative *)ptr;
        if (args[0].isClassInstance())
            dd->bindScriptOwner(vm, args[0]);
        else
            dd->bindScriptOwner(nullptr, Value());

        world->SetDebugDraw(dd);
        set_world_pin(vm, world, "debugdraw", args[0]);
        vm->pushBool(true);
        return 1;
    }

    static int b2world_debug_draw(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2World.debugDraw() expects 0 arguments");
            return push_nil1(vm);
        }

        b2World *world = nullptr;
        if (!get_world_from_instance(instance, &world, "B2World.debugDraw()"))
            return push_nil1(vm);

        world->DebugDraw();
        vm->pushBool(true);
        return 1;
    }

    static int b2world_set_contact_listener(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2World.setContactListener() expects (listenerOrNil)");
            return push_nil1(vm);
        }

        b2World *world = nullptr;
        if (!get_world_from_instance(instance, &world, "B2World.setContactListener()"))
            return push_nil1(vm);

        if (args[0].isNil())
        {
            world->SetContactListener(nullptr);
            clear_world_pin(vm, world, "contact");
            vm->pushBool(true);
            return 1;
        }

        void *ptr = nullptr;
        if (!get_native_value_ptr(args[0], kClassB2ContactListener, &ptr, "B2World.setContactListener()"))
            return push_nil1(vm);

        B2ContactListenerNative *listener = (B2ContactListenerNative *)ptr;
        if (args[0].isClassInstance())
            listener->bindScriptOwner(vm, args[0]);
        else
            listener->bindScriptOwner(nullptr, Value());

        world->SetContactListener(listener);
        set_world_pin(vm, world, "contact", args[0]);
        vm->pushBool(true);
        return 1;
    }

    static int b2world_query_aabb(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 5 || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber() || !args[4].isNumber())
        {
            Error("B2World.queryAABB() expects (callback, lowerX, lowerY, upperX, upperY)");
            return push_nil1(vm);
        }

        b2World *world = nullptr;
        if (!get_world_from_instance(instance, &world, "B2World.queryAABB()"))
            return push_nil1(vm);

        void *ptr = nullptr;
        if (!get_native_value_ptr(args[0], kClassB2QueryCallback, &ptr, "B2World.queryAABB()"))
            return push_nil1(vm);

        B2QueryCallbackNative *cb = (B2QueryCallbackNative *)ptr;
        if (args[0].isClassInstance())
            cb->bindScriptOwner(vm, args[0]);
        else
            cb->bindScriptOwner(nullptr, Value());

        const float x1 = (float)args[1].asNumber();
        const float y1 = (float)args[2].asNumber();
        const float x2 = (float)args[3].asNumber();
        const float y2 = (float)args[4].asNumber();

        b2AABB aabb;
        aabb.lowerBound.Set((x1 < x2) ? x1 : x2, (y1 < y2) ? y1 : y2);
        aabb.upperBound.Set((x1 > x2) ? x1 : x2, (y1 > y2) ? y1 : y2);

        cb->hitCount = 0;
        world->QueryAABB(cb, aabb);
        vm->pushInt(cb->hitCount);
        return 1;
    }

    static int b2world_ray_cast(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 5 || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber() || !args[4].isNumber())
        {
            Error("B2World.rayCast() expects (callback, x1, y1, x2, y2)");
            return push_nil1(vm);
        }

        b2World *world = nullptr;
        if (!get_world_from_instance(instance, &world, "B2World.rayCast()"))
            return push_nil1(vm);

        void *ptr = nullptr;
        if (!get_native_value_ptr(args[0], kClassB2RayCastCallback, &ptr, "B2World.rayCast()"))
            return push_nil1(vm);

        B2RayCastCallbackNative *cb = (B2RayCastCallbackNative *)ptr;
        if (args[0].isClassInstance())
            cb->bindScriptOwner(vm, args[0]);
        else
            cb->bindScriptOwner(nullptr, Value());

        const b2Vec2 p1((float)args[1].asNumber(), (float)args[2].asNumber());
        const b2Vec2 p2((float)args[3].asNumber(), (float)args[4].asNumber());

        cb->hitCount = 0;
        world->RayCast(cb, p1, p2);
        vm->pushInt(cb->hitCount);
        return 1;
    }

    // ---------------- B2Body ----------------

    static void *b2body_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)argCount;
        (void)args;
        Error("B2Body cannot be constructed directly; use B2World.createBody()");
        return nullptr;
    }

    static void b2body_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        (void)instance;
    }

    static int b2body_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2Body.destroy() expects 0 arguments");
            return push_nil1(vm);
        }

        b2Body *body = as_body(instance);
        if (!body)
        {
            return push_nil1(vm);
        }

        b2World *world = body->GetWorld();
        if (!world)
        {
            return push_nil1(vm);
        }
        g_b2BodyTags.erase(body);
        world->DestroyBody(body);
        vm->pushBool(true);
        return 1;
    }

    static int b2body_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2Body.isValid() expects 0 arguments");
            return push_nil1(vm);
        }
        vm->pushBool(as_body(instance) != nullptr);
        return 1;
    }

    static int b2body_create_fixture(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.createFixture()"))
            return push_nil1(vm);

        // Overload 1: createFixture(fixtureDef)
        if (argCount == 1)
        {
            void *ptr = nullptr;
            if (!get_native_value_ptr(args[0], kClassB2FixtureDef, &ptr, "B2Body.createFixture()"))
                return push_nil1(vm);
            b2FixtureDef *fd = (b2FixtureDef *)ptr;
            b2Fixture *fx = body->CreateFixture(fd);
            vm->pushBool(fx != nullptr);
            return 1;
        }

        // Overload 2: createFixture(shape, density)
        if (argCount == 2 && args[1].isNumber())
        {
            void *shapePtr = nullptr;
            if (try_get_native_value_ptr(args[0], kClassB2PolygonShape, &shapePtr))
            {
                b2Fixture *fx = body->CreateFixture((b2PolygonShape *)shapePtr, (float)args[1].asNumber());
                vm->pushBool(fx != nullptr);
                return 1;
            }
            if (try_get_native_value_ptr(args[0], kClassB2CircleShape, &shapePtr))
            {
                b2Fixture *fx = body->CreateFixture((b2CircleShape *)shapePtr, (float)args[1].asNumber());
                vm->pushBool(fx != nullptr);
                return 1;
            }
            if (try_get_native_value_ptr(args[0], kClassB2EdgeShape, &shapePtr))
            {
                b2Fixture *fx = body->CreateFixture((b2EdgeShape *)shapePtr, (float)args[1].asNumber());
                vm->pushBool(fx != nullptr);
                return 1;
            }
            if (try_get_native_value_ptr(args[0], kClassB2ChainShape, &shapePtr))
            {
                b2Fixture *fx = body->CreateFixture((b2ChainShape *)shapePtr, (float)args[1].asNumber());
                vm->pushBool(fx != nullptr);
                return 1;
            }
            Error("B2Body.createFixture() expects shape as B2PolygonShape/B2CircleShape/B2EdgeShape/B2ChainShape");
            return push_nil1(vm);
        }

        Error("B2Body.createFixture() expects (fixtureDef) or (shape, density)");
        return push_nil1(vm);
    }

    static int b2body_get_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2Body.getPosition() expects 0 arguments");
            return push_nil2(vm);
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.getPosition()"))
            return push_nil2(vm);
        const b2Vec2 p = body->GetPosition();
        vm->pushDouble(p.x);
        vm->pushDouble(p.y);
        return 2;
    }

    static int b2body_get_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2Body.getAngle() expects 0 arguments");
            return push_nil1(vm);
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.getAngle()"))
            return push_nil1(vm);
        vm->pushDouble(body->GetAngle());
        return 1;
    }

    static int b2body_set_linear_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("B2Body.setLinearVelocity() expects (vx, vy)");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.setLinearVelocity()"))
            return 0;
        body->SetLinearVelocity(b2Vec2((float)args[0].asNumber(), (float)args[1].asNumber()));
        return 0;
    }

    static int b2body_get_linear_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2Body.getLinearVelocity() expects 0 arguments");
            return push_nil2(vm);
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.getLinearVelocity()"))
            return push_nil2(vm);
        const b2Vec2 v = body->GetLinearVelocity();
        vm->pushDouble(v.x);
        vm->pushDouble(v.y);
        return 2;
    }

    static int b2body_apply_force_to_center(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2 && argCount != 3)
        {
            Error("B2Body.applyForceToCenter() expects (fx, fy[, wake])");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.applyForceToCenter()"))
            return 0;
        const bool wake = (argCount == 3) ? args[2].asBool() : true;
        body->ApplyForceToCenter(b2Vec2((float)args[0].asNumber(), (float)args[1].asNumber()), wake);
        return 0;
    }

    static int b2body_apply_linear_impulse_to_center(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2 && argCount != 3)
        {
            Error("B2Body.applyLinearImpulseToCenter() expects (ix, iy[, wake])");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.applyLinearImpulseToCenter()"))
            return 0;
        const bool wake = (argCount == 3) ? args[2].asBool() : true;
        body->ApplyLinearImpulseToCenter(b2Vec2((float)args[0].asNumber(), (float)args[1].asNumber()), wake);
        return 0;
    }

    static int b2body_set_transform(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("B2Body.setTransform() expects (x, y, angle)");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.setTransform()"))
            return 0;
        body->SetTransform(b2Vec2((float)args[0].asNumber(), (float)args[1].asNumber()), (float)args[2].asNumber());
        return 0;
    }

    static int b2body_set_type(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2Body.setType() expects (type)");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.setType()"))
            return 0;
        body->SetType((b2BodyType)args[0].asInt());
        return 0;
    }

    static int b2body_set_angular_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2Body.setAngularVelocity() expects (omega)");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.setAngularVelocity()"))
            return 0;
        body->SetAngularVelocity((float)args[0].asNumber());
        return 0;
    }

    static int b2body_set_fixed_rotation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2Body.setFixedRotation() expects (fixed)");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.setFixedRotation()"))
            return 0;
        body->SetFixedRotation(args[0].asBool());
        return 0;
    }

    static int b2body_set_awake(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2Body.setAwake() expects (awake)");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.setAwake()"))
            return 0;
        body->SetAwake(args[0].asBool());
        return 0;
    }

    static int b2body_get_mass(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2Body.getMass() expects 0 arguments");
            return push_nil1(vm);
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.getMass()"))
            return push_nil1(vm);
        vm->pushDouble(body->GetMass());
        return 1;
    }

    static int b2body_set_user_id(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2Body.setUserId() expects (idOrNil)");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.setUserId()"))
            return 0;

        if (args[0].isNil())
        {
            body->GetUserData().pointer = 0;
            return 0;
        }
        if (!args[0].isNumber())
        {
            Error("B2Body.setUserId() expects numeric id or nil");
            return 0;
        }

        double id = args[0].asNumber();
        if (!std::isfinite(id))
        {
            Error("B2Body.setUserId() expects finite number");
            return 0;
        }
        if (id < 0.0)
            id = 0.0;
        body->GetUserData().pointer = (uintptr_t)id;
        return 0;
    }

    static int b2body_get_user_id(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2Body.getUserId() expects 0 arguments");
            return push_nil1(vm);
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.getUserId()"))
            return push_nil1(vm);

        uintptr_t id = body->GetUserData().pointer;
        if (id == 0)
        {
            vm->pushNil();
            return 1;
        }
        vm->pushDouble((double)id);
        return 1;
    }

    static int b2body_set_tag(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2Body.setTag() expects (tagOrNil)");
            return 0;
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.setTag()"))
            return 0;

        if (args[0].isNil())
        {
            g_b2BodyTags.erase(body);
            return 0;
        }
        if (!args[0].isString())
        {
            Error("B2Body.setTag() expects string or nil");
            return 0;
        }

        const char *tag = args[0].asStringChars();
        g_b2BodyTags[body] = tag ? tag : "";
        return 0;
    }

    static int b2body_get_tag(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2Body.getTag() expects 0 arguments");
            return push_nil1(vm);
        }
        b2Body *body = nullptr;
        if (!get_body_from_instance(instance, &body, "B2Body.getTag()"))
            return push_nil1(vm);

        auto it = g_b2BodyTags.find(body);
        if (it == g_b2BodyTags.end())
        {
            vm->pushNil();
            return 1;
        }
        vm->pushString(it->second.c_str());
        return 1;
    }

    // ---------------- B2BodyDef ----------------

    static void *b2bodydef_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2BodyDef expects 0 arguments");
            return nullptr;
        }
        return new b2BodyDef();
    }

    static void b2bodydef_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_bodydef(instance);
    }

    static int b2bodydef_set_type(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2BodyDef.setType() expects (type)");
            return 0;
        }
        as_bodydef(instance)->type = (b2BodyType)args[0].asInt();
        return 0;
    }

    static int b2bodydef_set_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("B2BodyDef.setPosition() expects (x, y)");
            return 0;
        }
        as_bodydef(instance)->position.Set((float)args[0].asNumber(), (float)args[1].asNumber());
        return 0;
    }

    static int b2bodydef_set_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2BodyDef.setAngle() expects (angle)");
            return 0;
        }
        as_bodydef(instance)->angle = (float)args[0].asNumber();
        return 0;
    }

    static int b2bodydef_set_fixed_rotation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2BodyDef.setFixedRotation() expects (fixed)");
            return 0;
        }
        as_bodydef(instance)->fixedRotation = args[0].asBool();
        return 0;
    }

    static int b2bodydef_set_bullet(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2BodyDef.setBullet() expects (bullet)");
            return 0;
        }
        as_bodydef(instance)->bullet = args[0].asBool();
        return 0;
    }

    static int b2bodydef_set_awake(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2BodyDef.setAwake() expects (awake)");
            return 0;
        }
        as_bodydef(instance)->awake = args[0].asBool();
        return 0;
    }

    static int b2bodydef_set_gravity_scale(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2BodyDef.setGravityScale() expects (scale)");
            return 0;
        }
        as_bodydef(instance)->gravityScale = (float)args[0].asNumber();
        return 0;
    }

    // ---------------- B2PolygonShape ----------------

    static void *b2polygonshape_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2PolygonShape expects 0 arguments");
            return nullptr;
        }
        return new b2PolygonShape();
    }

    static void b2polygonshape_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_polygon_shape(instance);
    }

    static int b2polygonshape_set_as_box(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        b2PolygonShape *shape = as_polygon_shape(instance);
        if (argCount == 2)
        {
            shape->SetAsBox((float)args[0].asNumber(), (float)args[1].asNumber());
            return 0;
        }
        if (argCount == 5)
        {
            b2Vec2 center((float)args[2].asNumber(), (float)args[3].asNumber());
            shape->SetAsBox((float)args[0].asNumber(), (float)args[1].asNumber(), center, (float)args[4].asNumber());
            return 0;
        }
        Error("B2PolygonShape.setAsBox() expects (hx, hy) or (hx, hy, cx, cy, angle)");
        return 0;
    }

    // ---------------- B2CircleShape ----------------

    static void *b2circleshape_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2CircleShape expects 0 arguments");
            return nullptr;
        }
        return new b2CircleShape();
    }

    static void b2circleshape_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_circle_shape(instance);
    }

    static int b2circleshape_set_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2CircleShape.setRadius() expects (radius)");
            return 0;
        }
        as_circle_shape(instance)->m_radius = (float)args[0].asNumber();
        return 0;
    }

    static int b2circleshape_set_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("B2CircleShape.setPosition() expects (x, y)");
            return 0;
        }
        as_circle_shape(instance)->m_p.Set((float)args[0].asNumber(), (float)args[1].asNumber());
        return 0;
    }

    // ---------------- B2EdgeShape ----------------

    static void *b2edgeshape_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2EdgeShape expects 0 arguments");
            return nullptr;
        }
        return new b2EdgeShape();
    }

    static void b2edgeshape_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_edge_shape(instance);
    }

    static int b2edgeshape_set_two_sided(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 4)
        {
            Error("B2EdgeShape.setTwoSided() expects (x1, y1, x2, y2)");
            return 0;
        }
        as_edge_shape(instance)->SetTwoSided(
            b2Vec2((float)args[0].asNumber(), (float)args[1].asNumber()),
            b2Vec2((float)args[2].asNumber(), (float)args[3].asNumber()));
        return 0;
    }

    static int b2edgeshape_set_one_sided(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 8)
        {
            Error("B2EdgeShape.setOneSided() expects (x0, y0, x1, y1, x2, y2, x3, y3)");
            return 0;
        }
        as_edge_shape(instance)->SetOneSided(
            b2Vec2((float)args[0].asNumber(), (float)args[1].asNumber()),
            b2Vec2((float)args[2].asNumber(), (float)args[3].asNumber()),
            b2Vec2((float)args[4].asNumber(), (float)args[5].asNumber()),
            b2Vec2((float)args[6].asNumber(), (float)args[7].asNumber()));
        return 0;
    }

    // ---------------- B2ChainShape ----------------

    static void *b2chainshape_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2ChainShape expects 0 arguments");
            return nullptr;
        }
        return new b2ChainShape();
    }

    static void b2chainshape_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_chain_shape(instance);
    }

    static int b2chainshape_clear(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2ChainShape.clear() expects 0 arguments");
            return 0;
        }
        as_chain_shape(instance)->Clear();
        return 0;
    }

    static int b2chainshape_create_loop(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2ChainShape.createLoop() expects (verticesArray)");
            return 0;
        }

        std::vector<b2Vec2> verts;
        if (!read_vec2_array(args[0], &verts, "B2ChainShape.createLoop()", 1, 3))
            return 0;

        b2ChainShape *shape = as_chain_shape(instance);
        shape->Clear();
        shape->CreateLoop(verts.data(), (int32)verts.size());
        return 0;
    }

    static int b2chainshape_create_chain(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 5)
        {
            Error("B2ChainShape.createChain() expects (verticesArray) or (verticesArray, prevX, prevY, nextX, nextY)");
            return 0;
        }

        std::vector<b2Vec2> verts;
        if (!read_vec2_array(args[0], &verts, "B2ChainShape.createChain()", 1, 2))
            return 0;

        b2Vec2 prev = verts.front();
        b2Vec2 next = verts.back();
        if (argCount == 5)
        {
            prev.Set((float)args[1].asNumber(), (float)args[2].asNumber());
            next.Set((float)args[3].asNumber(), (float)args[4].asNumber());
        }

        b2ChainShape *shape = as_chain_shape(instance);
        shape->Clear();
        shape->CreateChain(verts.data(), (int32)verts.size(), prev, next);
        return 0;
    }

    // ---------------- B2FixtureDef ----------------

    static void *b2fixturedef_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2FixtureDef expects 0 arguments");
            return nullptr;
        }
        return new b2FixtureDef();
    }

    static void b2fixturedef_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_fixturedef(instance);
    }

    static int b2fixturedef_set_shape(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2FixtureDef.setShape() expects (shape)");
            return 0;
        }

        b2FixtureDef *fd = as_fixturedef(instance);
        void *shapePtr = nullptr;
        if (try_get_native_value_ptr(args[0], kClassB2PolygonShape, &shapePtr))
        {
            fd->shape = (b2PolygonShape *)shapePtr;
            return 0;
        }
        if (try_get_native_value_ptr(args[0], kClassB2CircleShape, &shapePtr))
        {
            fd->shape = (b2CircleShape *)shapePtr;
            return 0;
        }
        if (try_get_native_value_ptr(args[0], kClassB2EdgeShape, &shapePtr))
        {
            fd->shape = (b2EdgeShape *)shapePtr;
            return 0;
        }
        if (try_get_native_value_ptr(args[0], kClassB2ChainShape, &shapePtr))
        {
            fd->shape = (b2ChainShape *)shapePtr;
            return 0;
        }

        Error("B2FixtureDef.setShape() expects B2PolygonShape, B2CircleShape, B2EdgeShape or B2ChainShape");
        return 0;
    }

    static int b2fixturedef_set_density(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2FixtureDef.setDensity() expects (density)");
            return 0;
        }
        as_fixturedef(instance)->density = (float)args[0].asNumber();
        return 0;
    }

    static int b2fixturedef_set_friction(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2FixtureDef.setFriction() expects (friction)");
            return 0;
        }
        as_fixturedef(instance)->friction = (float)args[0].asNumber();
        return 0;
    }

    static int b2fixturedef_set_restitution(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2FixtureDef.setRestitution() expects (restitution)");
            return 0;
        }
        as_fixturedef(instance)->restitution = (float)args[0].asNumber();
        return 0;
    }

    static int b2fixturedef_set_sensor(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2FixtureDef.setSensor() expects (sensor)");
            return 0;
        }
        as_fixturedef(instance)->isSensor = args[0].asBool();
        return 0;
    }

    static int b2fixturedef_set_category_bits(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("B2FixtureDef.setCategoryBits() expects (bits)");
            return 0;
        }
        int bits = (int)args[0].asNumber();
        if (bits < 0)
            bits = 0;
        if (bits > 0xFFFF)
            bits = 0xFFFF;
        as_fixturedef(instance)->filter.categoryBits = (uint16)bits;
        return 0;
    }

    static int b2fixturedef_set_mask_bits(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("B2FixtureDef.setMaskBits() expects (bits)");
            return 0;
        }
        int bits = (int)args[0].asNumber();
        if (bits < 0)
            bits = 0;
        if (bits > 0xFFFF)
            bits = 0xFFFF;
        as_fixturedef(instance)->filter.maskBits = (uint16)bits;
        return 0;
    }

    static int b2fixturedef_set_group_index(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("B2FixtureDef.setGroupIndex() expects (groupIndex)");
            return 0;
        }
        int idx = (int)args[0].asNumber();
        if (idx < -32768)
            idx = -32768;
        if (idx > 32767)
            idx = 32767;
        as_fixturedef(instance)->filter.groupIndex = (int16)idx;
        return 0;
    }

    // ---------------- B2ContactListener ----------------

    static void *b2contact_listener_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2ContactListener expects 0 arguments");
            return nullptr;
        }
        return new B2ContactListenerNative();
    }

    static void b2contact_listener_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_contact_listener(instance);
    }

    // ---------------- B2QueryCallback ----------------

    static void *b2query_callback_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2QueryCallback expects 0 arguments");
            return nullptr;
        }
        return new B2QueryCallbackNative();
    }

    static void b2query_callback_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_query_callback(instance);
    }

    // ---------------- B2RayCastCallback ----------------

    static void *b2raycast_callback_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2RayCastCallback expects 0 arguments");
            return nullptr;
        }
        return new B2RayCastCallbackNative();
    }

    static void b2raycast_callback_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_raycast_callback(instance);
    }

    // ---------------- B2DebugDraw ----------------

    static void *b2debugdraw_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("B2DebugDraw expects 0 arguments");
            return nullptr;
        }

        B2DebugDrawNative *dd = new B2DebugDrawNative();
        dd->SetFlags(b2Draw::e_shapeBit);
        return dd;
    }

    static void b2debugdraw_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete as_debugdraw(instance);
    }

    static int b2debugdraw_set_flags(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2DebugDraw.setFlags() expects (flags)");
            return 0;
        }
        as_debugdraw(instance)->SetFlags((uint32)args[0].asInt());
        return 0;
    }

    static int b2debugdraw_get_flags(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2DebugDraw.getFlags() expects 0 arguments");
            return push_nil1(vm);
        }
        vm->pushInt((int)as_debugdraw(instance)->GetFlags());
        return 1;
    }

    static int b2debugdraw_append_flags(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2DebugDraw.appendFlags() expects (flags)");
            return 0;
        }
        as_debugdraw(instance)->AppendFlags((uint32)args[0].asInt());
        return 0;
    }

    static int b2debugdraw_clear_flags(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("B2DebugDraw.clearFlags() expects (flags)");
            return 0;
        }
        as_debugdraw(instance)->ClearFlags((uint32)args[0].asInt());
        return 0;
    }

    static int b2debugdraw_set_line_width(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("B2DebugDraw.setLineWidth() expects (value)");
            return 0;
        }
        float value = (float)args[0].asNumber();
        if (value <= 0.0f)
            value = 1.0f;
        as_debugdraw(instance)->lineWidth = value;
        return 0;
    }

    static int b2debugdraw_get_line_width(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2DebugDraw.getLineWidth() expects 0 arguments");
            return push_nil1(vm);
        }
        vm->pushDouble(as_debugdraw(instance)->lineWidth);
        return 1;
    }

    static int b2debugdraw_set_fill_alpha(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("B2DebugDraw.setFillAlpha() expects (value)");
            return 0;
        }
        float value = (float)args[0].asNumber();
        if (value < 0.0f)
            value = 0.0f;
        if (value > 1.0f)
            value = 1.0f;
        as_debugdraw(instance)->fillAlpha = value;
        return 0;
    }

    static int b2debugdraw_get_fill_alpha(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2DebugDraw.getFillAlpha() expects 0 arguments");
            return push_nil1(vm);
        }
        vm->pushDouble(as_debugdraw(instance)->fillAlpha);
        return 1;
    }

    static int b2debugdraw_set_axis_scale(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("B2DebugDraw.setAxisScale() expects (value)");
            return 0;
        }
        float value = (float)args[0].asNumber();
        if (value <= 0.0f)
            value = 0.40f;
        as_debugdraw(instance)->axisScale = value;
        return 0;
    }

    static int b2debugdraw_get_axis_scale(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2DebugDraw.getAxisScale() expects 0 arguments");
            return push_nil1(vm);
        }
        vm->pushDouble(as_debugdraw(instance)->axisScale);
        return 1;
    }

    static int b2debugdraw_set_point_size(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("B2DebugDraw.setPointSize() expects (value)");
            return 0;
        }
        float value = (float)args[0].asNumber();
        if (value <= 0.0f)
            value = 4.0f;
        as_debugdraw(instance)->pointSize = value;
        return 0;
    }

    static int b2debugdraw_get_point_size(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("B2DebugDraw.getPointSize() expects 0 arguments");
            return push_nil1(vm);
        }
        vm->pushDouble(as_debugdraw(instance)->pointSize);
        return 1;
    }
    
    



 

    void register_box2d(Interpreter &vm)
    {
        NativeClassDef *worldClass = vm.registerNativeClass(kClassB2World, b2world_ctor, b2world_dtor, 2, false);
        g_b2BodyClass = vm.registerNativeClass(kClassB2Body, b2body_ctor, b2body_dtor, 0, false);
        NativeClassDef *bodyDefClass = vm.registerNativeClass(kClassB2BodyDef, b2bodydef_ctor, b2bodydef_dtor, 0, false);
        NativeClassDef *polyShapeClass = vm.registerNativeClass(kClassB2PolygonShape, b2polygonshape_ctor, b2polygonshape_dtor, 0, false);
        NativeClassDef *circleShapeClass = vm.registerNativeClass(kClassB2CircleShape, b2circleshape_ctor, b2circleshape_dtor, 0, false);
        NativeClassDef *edgeShapeClass = vm.registerNativeClass(kClassB2EdgeShape, b2edgeshape_ctor, b2edgeshape_dtor, 0, false);
        NativeClassDef *chainShapeClass = vm.registerNativeClass(kClassB2ChainShape, b2chainshape_ctor, b2chainshape_dtor, 0, false);
        NativeClassDef *fixtureDefClass = vm.registerNativeClass(kClassB2FixtureDef, b2fixturedef_ctor, b2fixturedef_dtor, 0, false);
        g_b2ContactListenerClass = vm.registerNativeClass(kClassB2ContactListener, b2contact_listener_ctor, b2contact_listener_dtor, 0, false);
        g_b2QueryCallbackClass = vm.registerNativeClass(kClassB2QueryCallback, b2query_callback_ctor, b2query_callback_dtor, 0, false);
        g_b2RayCastCallbackClass = vm.registerNativeClass(kClassB2RayCastCallback, b2raycast_callback_ctor, b2raycast_callback_dtor, 0, false);
        g_b2DebugDrawClass = vm.registerNativeClass(kClassB2DebugDraw, b2debugdraw_ctor, b2debugdraw_dtor, 0, false);

        vm.addNativeMethod(worldClass, "destroy", b2world_destroy);
        vm.addNativeMethod(worldClass, "isValid", b2world_is_valid);
        vm.addNativeMethod(worldClass, "step", b2world_step);
        vm.addNativeMethod(worldClass, "createBody", b2world_create_body);
        vm.addNativeMethod(worldClass, "setDebugDraw", b2world_set_debug_draw);
        vm.addNativeMethod(worldClass, "debugDraw", b2world_debug_draw);
        vm.addNativeMethod(worldClass, "setContactListener", b2world_set_contact_listener);
        vm.addNativeMethod(worldClass, "queryAABB", b2world_query_aabb);
        vm.addNativeMethod(worldClass, "rayCast", b2world_ray_cast);

        vm.addNativeMethod(g_b2BodyClass, "destroy", b2body_destroy);
        vm.addNativeMethod(g_b2BodyClass, "isValid", b2body_is_valid);
        vm.addNativeMethod(g_b2BodyClass, "createFixture", b2body_create_fixture);
        vm.addNativeMethod(g_b2BodyClass, "getPosition", b2body_get_position);
        vm.addNativeMethod(g_b2BodyClass, "getAngle", b2body_get_angle);
        vm.addNativeMethod(g_b2BodyClass, "setLinearVelocity", b2body_set_linear_velocity);
        vm.addNativeMethod(g_b2BodyClass, "getLinearVelocity", b2body_get_linear_velocity);
        vm.addNativeMethod(g_b2BodyClass, "applyForceToCenter", b2body_apply_force_to_center);
        vm.addNativeMethod(g_b2BodyClass, "applyLinearImpulseToCenter", b2body_apply_linear_impulse_to_center);
        vm.addNativeMethod(g_b2BodyClass, "setTransform", b2body_set_transform);
        vm.addNativeMethod(g_b2BodyClass, "setType", b2body_set_type);
        vm.addNativeMethod(g_b2BodyClass, "setAngularVelocity", b2body_set_angular_velocity);
        vm.addNativeMethod(g_b2BodyClass, "setFixedRotation", b2body_set_fixed_rotation);
        vm.addNativeMethod(g_b2BodyClass, "setAwake", b2body_set_awake);
        vm.addNativeMethod(g_b2BodyClass, "getMass", b2body_get_mass);
        vm.addNativeMethod(g_b2BodyClass, "setUserId", b2body_set_user_id);
        vm.addNativeMethod(g_b2BodyClass, "getUserId", b2body_get_user_id);
        vm.addNativeMethod(g_b2BodyClass, "setTag", b2body_set_tag);
        vm.addNativeMethod(g_b2BodyClass, "getTag", b2body_get_tag);

        vm.addNativeMethod(bodyDefClass, "setType", b2bodydef_set_type);
        vm.addNativeMethod(bodyDefClass, "setPosition", b2bodydef_set_position);
        vm.addNativeMethod(bodyDefClass, "setAngle", b2bodydef_set_angle);
        vm.addNativeMethod(bodyDefClass, "setFixedRotation", b2bodydef_set_fixed_rotation);
        vm.addNativeMethod(bodyDefClass, "setBullet", b2bodydef_set_bullet);
        vm.addNativeMethod(bodyDefClass, "setAwake", b2bodydef_set_awake);
        vm.addNativeMethod(bodyDefClass, "setGravityScale", b2bodydef_set_gravity_scale);

        vm.addNativeMethod(polyShapeClass, "setAsBox", b2polygonshape_set_as_box);

        vm.addNativeMethod(circleShapeClass, "setRadius", b2circleshape_set_radius);
        vm.addNativeMethod(circleShapeClass, "setPosition", b2circleshape_set_position);
        vm.addNativeMethod(edgeShapeClass, "setTwoSided", b2edgeshape_set_two_sided);
        vm.addNativeMethod(edgeShapeClass, "setOneSided", b2edgeshape_set_one_sided);
        vm.addNativeMethod(chainShapeClass, "clear", b2chainshape_clear);
        vm.addNativeMethod(chainShapeClass, "createLoop", b2chainshape_create_loop);
        vm.addNativeMethod(chainShapeClass, "createChain", b2chainshape_create_chain);

        vm.addNativeMethod(fixtureDefClass, "setShape", b2fixturedef_set_shape);
        vm.addNativeMethod(fixtureDefClass, "setDensity", b2fixturedef_set_density);
        vm.addNativeMethod(fixtureDefClass, "setFriction", b2fixturedef_set_friction);
        vm.addNativeMethod(fixtureDefClass, "setRestitution", b2fixturedef_set_restitution);
        vm.addNativeMethod(fixtureDefClass, "setSensor", b2fixturedef_set_sensor);
        vm.addNativeMethod(fixtureDefClass, "setCategoryBits", b2fixturedef_set_category_bits);
        vm.addNativeMethod(fixtureDefClass, "setMaskBits", b2fixturedef_set_mask_bits);
        vm.addNativeMethod(fixtureDefClass, "setGroupIndex", b2fixturedef_set_group_index);

        vm.addNativeMethod(g_b2DebugDrawClass, "setFlags", b2debugdraw_set_flags);
        vm.addNativeMethod(g_b2DebugDrawClass, "getFlags", b2debugdraw_get_flags);
        vm.addNativeMethod(g_b2DebugDrawClass, "appendFlags", b2debugdraw_append_flags);
        vm.addNativeMethod(g_b2DebugDrawClass, "clearFlags", b2debugdraw_clear_flags);
        vm.addNativeMethod(g_b2DebugDrawClass, "setLineWidth", b2debugdraw_set_line_width);
        vm.addNativeMethod(g_b2DebugDrawClass, "getLineWidth", b2debugdraw_get_line_width);
        vm.addNativeMethod(g_b2DebugDrawClass, "setFillAlpha", b2debugdraw_set_fill_alpha);
        vm.addNativeMethod(g_b2DebugDrawClass, "getFillAlpha", b2debugdraw_get_fill_alpha);
        vm.addNativeMethod(g_b2DebugDrawClass, "setAxisScale", b2debugdraw_set_axis_scale);
        vm.addNativeMethod(g_b2DebugDrawClass, "getAxisScale", b2debugdraw_get_axis_scale);
        vm.addNativeMethod(g_b2DebugDrawClass, "setPointSize", b2debugdraw_set_point_size);
        vm.addNativeMethod(g_b2DebugDrawClass, "getPointSize", b2debugdraw_get_point_size);
        

        ModuleBuilder module = vm.addModule("Box2D");
        module
            .addInt("B2_STATIC_BODY", (int)b2_staticBody)
            .addInt("B2_KINEMATIC_BODY", (int)b2_kinematicBody)
            .addInt("B2_DYNAMIC_BODY", (int)b2_dynamicBody)
            .addInt("B2_DRAW_SHAPE_BIT", (int)b2Draw::e_shapeBit)
            .addInt("B2_DRAW_JOINT_BIT", (int)b2Draw::e_jointBit)
            .addInt("B2_DRAW_AABB_BIT", (int)b2Draw::e_aabbBit)
            .addInt("B2_DRAW_PAIR_BIT", (int)b2Draw::e_pairBit)
            .addInt("B2_DRAW_CENTER_OF_MASS_BIT", (int)b2Draw::e_centerOfMassBit);
    }
}
