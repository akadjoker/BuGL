#include "bindings.hpp"
#include "interpreter.hpp"

#include <poly2tri.h>

#include <cmath>
#include <exception>
#include <vector>

#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define BUGAME_HAS_CPP_EXCEPTIONS 1
#else
#define BUGAME_HAS_CPP_EXCEPTIONS 0
#endif

namespace SDLBindings
{
    static constexpr const char *kClassCDT = "CDT";

    struct RawPoint
    {
        double x;
        double y;
    };

    static bool same_point(const RawPoint &a, const RawPoint &b)
    {
        static constexpr double kEps = 1e-9;
        return std::fabs(a.x - b.x) <= kEps && std::fabs(a.y - b.y) <= kEps;
    }

    static bool is_collinear(const RawPoint &a, const RawPoint &b, const RawPoint &c)
    {
        static constexpr double kEps = 1e-9;
        const double cross = ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
        return std::fabs(cross) <= kEps;
    }

    static bool simplify_ring_points(const std::vector<RawPoint> &in, std::vector<RawPoint> *out)
    {
        if (!out)
            return false;
        out->clear();
        if (in.empty())
            return false;

        out->reserve(in.size());
        for (const RawPoint &p : in)
        {
            if (out->empty() || !same_point(out->back(), p))
            {
                out->push_back(p);
            }
        }

        if (out->size() >= 2 && same_point(out->front(), out->back()))
        {
            out->pop_back();
        }
        if (out->size() < 3)
            return false;

        bool changed = true;
        int pass = 0;
        while (changed && out->size() >= 3 && pass < 64)
        {
            changed = false;
            std::vector<RawPoint> next;
            const size_t n = out->size();
            next.reserve(n);

            for (size_t i = 0; i < n; ++i)
            {
                const RawPoint &prev = (*out)[(i + n - 1) % n];
                const RawPoint &curr = (*out)[i];
                const RawPoint &nextPoint = (*out)[(i + 1) % n];

                if (same_point(prev, curr) || same_point(curr, nextPoint) || is_collinear(prev, curr, nextPoint))
                {
                    changed = true;
                    continue;
                }
                next.push_back(curr);
            }

            if (next.size() < 3)
            {
                out->clear();
                return false;
            }

            if (next.size() != out->size())
            {
                *out = next;
            }
            pass++;
        }

        return out->size() >= 3;
    }

    class CDTInstance
    {
    public:
        CDTInstance() = default;

        bool initialize(const std::vector<p2t::Point *> &polyline)
        {
#if BUGAME_HAS_CPP_EXCEPTIONS
            try
            {
                cdt = new p2t::CDT(polyline);
                return true;
            }
            catch (const std::exception &e)
            {
                Error("CDT init failed: %s", e.what());
                return false;
            }
            catch (...)
            {
                Error("CDT init failed with unknown exception");
                return false;
            }
#else
            cdt = new p2t::CDT(polyline);
            return true;
#endif
        }

        ~CDTInstance()
        {
            delete cdt;
            cdt = nullptr;
            for (p2t::Point *p : ownedPoints)
            {
                delete p;
            }
            ownedPoints.clear();
        }

        p2t::Point *newPoint(double x, double y)
        {
            p2t::Point *p = new p2t::Point(x, y);
            ownedPoints.push_back(p);
            return p;
        }

        p2t::CDT *cdt = nullptr;

    private:
        std::vector<p2t::Point *> ownedPoints;
    };

    static NativeClassDef *require_native_class(Interpreter *vm, const char *className)
    {
        NativeClassDef *klass = nullptr;
        if (!vm || !vm->tryGetNativeClassDef(className, &klass) || !klass)
        {
            Error("%s class is not registered !", className);
            return nullptr;
        }
        return klass;
    }

    static bool push_native_instance(Interpreter *vm, const char *className, void *userData)
    {
        NativeClassDef *klass = require_native_class(vm, className);
        if (!klass || !userData)
        {
            vm->pushNil();
            return false;
        }

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *instance = value.asNativeClassInstance();
        instance->klass = klass;
        instance->userData = userData;
        vm->push(value);
        return true;
    }

    static CDTInstance *as_cdt(void *data, const char *funcName)
    {
        CDTInstance *ctx = static_cast<CDTInstance *>(data);
        if (!ctx || !ctx->cdt)
        {
            Error("%s on null CDT instance", funcName);
            return nullptr;
        }
        return ctx;
    }

    static bool parse_points_array(Interpreter *vm, CDTInstance *ctx, const Value &value, std::vector<p2t::Point *> *out, const char *funcName)
    {
        (void)vm;
        if (!ctx || !out)
            return false;
        out->clear();

        if (!value.isArray())
        {
            Error("%s expects array [x0,y0,x1,y1,...]", funcName);
            return false;
        }

        ArrayInstance *arr = value.asArray();
        if ((arr->values.size() % 2) != 0)
        {
            Error("%s expects even number of values", funcName);
            return false;
        }

        const size_t rawPointCount = arr->values.size() / 2;
        if (rawPointCount < 3)
        {
            Error("%s expects at least 3 points", funcName);
            return false;
        }

        std::vector<RawPoint> raw;
        raw.reserve(rawPointCount);
        for (size_t i = 0; i < arr->values.size(); i += 2)
        {
            const Value &vx = arr->values[i];
            const Value &vy = arr->values[i + 1];
            if (!vx.isNumber() || !vy.isNumber())
            {
                Error("%s expects numeric values", funcName);
                return false;
            }
            raw.push_back({vx.asNumber(), vy.asNumber()});
        }

        std::vector<RawPoint> clean;
        if (!simplify_ring_points(raw, &clean))
        {
            Error("%s invalid polygon/hole: need at least 3 non-collinear points", funcName);
            return false;
        }

        out->reserve(clean.size());
        for (const RawPoint &p : clean)
        {
            out->push_back(ctx->newPoint(p.x, p.y));
        }

        return true;
    }

    static void push_triangles_array(Interpreter *vm, const std::vector<p2t::Triangle *> &tris)
    {
        Value out = vm->makeArray();
        ArrayInstance *arr = out.asArray();
        arr->values.reserve(tris.size() * 6);

        for (p2t::Triangle *tri : tris)
        {
            if (!tri)
                continue;
            p2t::Point *a = tri->GetPoint(0);
            p2t::Point *b = tri->GetPoint(1);
            p2t::Point *c = tri->GetPoint(2);
            if (!a || !b || !c)
                continue;

            arr->values.push(vm->makeDouble(a->x));
            arr->values.push(vm->makeDouble(a->y));
            arr->values.push(vm->makeDouble(b->x));
            arr->values.push(vm->makeDouble(b->y));
            arr->values.push(vm->makeDouble(c->x));
            arr->values.push(vm->makeDouble(c->y));
        }

        vm->push(out);
    }

    static void *ctor_cdt(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("CDT expects 1 argument (polyline)");
            return nullptr;
        }

        CDTInstance *ctx = new CDTInstance();
        std::vector<p2t::Point *> polyline;
        if (!parse_points_array(vm, ctx, args[0], &polyline, "CDT"))
        {
            delete ctx;
            return nullptr;
        }

        if (!ctx->initialize(polyline))
        {
            delete ctx;
            return nullptr;
        }

        return ctx;
    }

    static void dtor_cdt(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<CDTInstance *>(data);
    }

    static int native_cdt_add_hole(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("AddHole expects 1 argument (polyline)");
            return 0;
        }

        CDTInstance *ctx = as_cdt(data, "AddHole");
        if (!ctx)
            return 0;

        std::vector<p2t::Point *> hole;
        if (!parse_points_array(vm, ctx, args[0], &hole, "AddHole"))
            return 0;

#if BUGAME_HAS_CPP_EXCEPTIONS
        try
        {
            ctx->cdt->AddHole(hole);
        }
        catch (const std::exception &e)
        {
            Error("AddHole failed: %s", e.what());
        }
        catch (...)
        {
            Error("AddHole failed with unknown exception");
        }
#else
        ctx->cdt->AddHole(hole);
#endif
        return 0;
    }

    static int native_cdt_add_point(Interpreter *vm, void *data, int argCount, Value *args)
    {
        CDTInstance *ctx = as_cdt(data, "AddPoint");
        if (!ctx)
            return 0;

        if (argCount == 2 && args[0].isNumber() && args[1].isNumber())
        {
#if BUGAME_HAS_CPP_EXCEPTIONS
            try
            {
                ctx->cdt->AddPoint(ctx->newPoint(args[0].asNumber(), args[1].asNumber()));
            }
            catch (const std::exception &e)
            {
                Error("AddPoint failed: %s", e.what());
            }
            catch (...)
            {
                Error("AddPoint failed with unknown exception");
            }
#else
            ctx->cdt->AddPoint(ctx->newPoint(args[0].asNumber(), args[1].asNumber()));
#endif
            return 0;
        }

        if (argCount == 1 && args[0].isArray())
        {
            ArrayInstance *arr = args[0].asArray();
            if (arr->values.size() != 2 || !arr->values[0].isNumber() || !arr->values[1].isNumber())
            {
                Error("AddPoint expects [x,y] or (x,y)");
                return 0;
            }
#if BUGAME_HAS_CPP_EXCEPTIONS
            try
            {
                ctx->cdt->AddPoint(ctx->newPoint(arr->values[0].asNumber(), arr->values[1].asNumber()));
            }
            catch (const std::exception &e)
            {
                Error("AddPoint failed: %s", e.what());
            }
            catch (...)
            {
                Error("AddPoint failed with unknown exception");
            }
#else
            ctx->cdt->AddPoint(ctx->newPoint(arr->values[0].asNumber(), arr->values[1].asNumber()));
#endif
            return 0;
        }

        Error("AddPoint expects [x,y] or (x,y)");
        return 0;
    }

    static int native_cdt_triangulate(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("Triangulate expects no arguments");
            return 0;
        }

        CDTInstance *ctx = as_cdt(data, "Triangulate");
        if (!ctx)
            return 0;

#if BUGAME_HAS_CPP_EXCEPTIONS
        try
        {
            ctx->cdt->Triangulate();
        }
        catch (const std::exception &e)
        {
            Error("Triangulate failed: %s", e.what());
        }
        catch (...)
        {
            Error("Triangulate failed with unknown exception");
        }
#else
        ctx->cdt->Triangulate();
#endif
        return 0;
    }

    static int native_cdt_get_triangles(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("GetTriangles expects no arguments");
            vm->pushNil();
            return 1;
        }

        CDTInstance *ctx = as_cdt(data, "GetTriangles");
        if (!ctx)
        {
            vm->pushNil();
            return 1;
        }

        push_triangles_array(vm, ctx->cdt->GetTriangles());
        return 1;
    }

    void register_poly2tri(Interpreter &vm)
    {
        NativeClassDef *CDT = vm.registerNativeClass(
            kClassCDT,
            ctor_cdt,
            dtor_cdt,
            1,
            false);

        vm.addNativeMethod(CDT, "add_hole", native_cdt_add_hole);
        vm.addNativeMethod(CDT, "add_point", native_cdt_add_point);
        vm.addNativeMethod(CDT, "triangulate", native_cdt_triangulate);
        vm.addNativeMethod(CDT, "get_triangles", native_cdt_get_triangles);
    }
}
