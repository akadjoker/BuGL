#include "bindings.hpp"
#include "raymath.h"
#include <math.h>
 

namespace BindingsEase
{
namespace
{
    static float clamp01(float t)
    {
        if (t < 0.0f)
            return 0.0f;
        if (t > 1.0f)
            return 1.0f;
        return t;
    }

    static int pushEasedValue(Interpreter *vm, int argCount, Value *args, const char *name, float (*fn)(float))
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("%s expects 1 number argument (t in [0..1])", name);
            vm->pushDouble(0.0);
            return 1;
        }

        float t = clamp01((float)args[0].asNumber());
        vm->pushDouble((double)fn(t));
        return 1;
    }

    static float easeLinear(float t)
    {
        return t;
    }

    static float easeSineIn(float t)
    {
        return 1.0f - cosf((t * PI) * 0.5f);
    }

    static float easeSineOut(float t)
    {
        return sinf((t * PI) * 0.5f);
    }

    static float easeSineInOut(float t)
    {
        return -(cosf(PI * t) - 1.0f) * 0.5f;
    }

    static float easeQuadIn(float t)
    {
        return t * t;
    }

    static float easeQuadOut(float t)
    {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    static float easeQuadInOut(float t)
    {
        if (t < 0.5f)
            return 2.0f * t * t;
        float v = -2.0f * t + 2.0f;
        return 1.0f - (v * v) * 0.5f;
    }

    static float easeCubicIn(float t)
    {
        return t * t * t;
    }

    static float easeCubicOut(float t)
    {
        float v = 1.0f - t;
        return 1.0f - v * v * v;
    }

    static float easeCubicInOut(float t)
    {
        if (t < 0.5f)
            return 4.0f * t * t * t;
        float v = -2.0f * t + 2.0f;
        return 1.0f - (v * v * v) * 0.5f;
    }

    static float easeQuartIn(float t)
    {
        return t * t * t * t;
    }

    static float easeQuartOut(float t)
    {
        float v = 1.0f - t;
        return 1.0f - v * v * v * v;
    }

    static float easeQuartInOut(float t)
    {
        if (t < 0.5f)
            return 8.0f * t * t * t * t;
        float v = -2.0f * t + 2.0f;
        return 1.0f - (v * v * v * v) * 0.5f;
    }

    static float easeQuintIn(float t)
    {
        return t * t * t * t * t;
    }

    static float easeQuintOut(float t)
    {
        float v = 1.0f - t;
        return 1.0f - v * v * v * v * v;
    }

    static float easeQuintInOut(float t)
    {
        if (t < 0.5f)
            return 16.0f * t * t * t * t * t;
        float v = -2.0f * t + 2.0f;
        return 1.0f - (v * v * v * v * v) * 0.5f;
    }

    static float easeExpoIn(float t)
    {
        if (t <= 0.0f)
            return 0.0f;
        return powf(2.0f, 10.0f * t - 10.0f);
    }

    static float easeExpoOut(float t)
    {
        if (t >= 1.0f)
            return 1.0f;
        return 1.0f - powf(2.0f, -10.0f * t);
    }

    static float easeExpoInOut(float t)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;
        if (t < 0.5f)
            return powf(2.0f, 20.0f * t - 10.0f) * 0.5f;
        return (2.0f - powf(2.0f, -20.0f * t + 10.0f)) * 0.5f;
    }

    static float easeCircIn(float t)
    {
        return 1.0f - sqrtf(1.0f - t * t);
    }

    static float easeCircOut(float t)
    {
        float v = t - 1.0f;
        return sqrtf(1.0f - v * v);
    }

    static float easeCircInOut(float t)
    {
        if (t < 0.5f)
        {
            float v = 2.0f * t;
            return (1.0f - sqrtf(1.0f - v * v)) * 0.5f;
        }
        float v = -2.0f * t + 2.0f;
        return (sqrtf(1.0f - v * v) + 1.0f) * 0.5f;
    }

    static float easeBackIn(float t)
    {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        return c3 * t * t * t - c1 * t * t;
    }

    static float easeBackOut(float t)
    {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        float v = t - 1.0f;
        return 1.0f + c3 * v * v * v + c1 * v * v;
    }

    static float easeBackInOut(float t)
    {
        const float c1 = 1.70158f;
        const float c2 = c1 * 1.525f;
        if (t < 0.5f)
        {
            float v = 2.0f * t;
            return (v * v * ((c2 + 1.0f) * v - c2)) * 0.5f;
        }
        float v = 2.0f * t - 2.0f;
        return (v * v * ((c2 + 1.0f) * v + c2) + 2.0f) * 0.5f;
    }

    static float easeElasticIn(float t)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;
        const float c4 = (2.0f * PI) / 3.0f;
        return -powf(2.0f, 10.0f * t - 10.0f) * sinf((t * 10.0f - 10.75f) * c4);
    }

    static float easeElasticOut(float t)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;
        const float c4 = (2.0f * PI) / 3.0f;
        return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
    }

    static float easeElasticInOut(float t)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;
        const float c5 = (2.0f * PI) / 4.5f;
        if (t < 0.5f)
            return -(powf(2.0f, 20.0f * t - 10.0f) * sinf((20.0f * t - 11.125f) * c5)) * 0.5f;
        return (powf(2.0f, -20.0f * t + 10.0f) * sinf((20.0f * t - 11.125f) * c5)) * 0.5f + 1.0f;
    }

    static float easeBounceOut(float t)
    {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;

        if (t < 1.0f / d1)
            return n1 * t * t;
        if (t < 2.0f / d1)
        {
            t -= 1.5f / d1;
            return n1 * t * t + 0.75f;
        }
        if (t < 2.5f / d1)
        {
            t -= 2.25f / d1;
            return n1 * t * t + 0.9375f;
        }
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }

    static float easeBounceIn(float t)
    {
        return 1.0f - easeBounceOut(1.0f - t);
    }

    static float easeBounceInOut(float t)
    {
        if (t < 0.5f)
            return (1.0f - easeBounceOut(1.0f - 2.0f * t)) * 0.5f;
        return (1.0f + easeBounceOut(2.0f * t - 1.0f)) * 0.5f;
    }
}

#define DEFINE_EASE_NATIVE(native_name, script_name, ease_fn)            \
    static int native_name(Interpreter *vm, int argCount, Value *args)   \
    {                                                                     \
        return pushEasedValue(vm, argCount, args, script_name, ease_fn); \
    }

DEFINE_EASE_NATIVE(native_ease_linear, "ease_linear", easeLinear)
DEFINE_EASE_NATIVE(native_ease_sine_in, "ease_sine_in", easeSineIn)
DEFINE_EASE_NATIVE(native_ease_sine_out, "ease_sine_out", easeSineOut)
DEFINE_EASE_NATIVE(native_ease_sine_in_out, "ease_sine_in_out", easeSineInOut)
DEFINE_EASE_NATIVE(native_ease_quad_in, "ease_quad_in", easeQuadIn)
DEFINE_EASE_NATIVE(native_ease_quad_out, "ease_quad_out", easeQuadOut)
DEFINE_EASE_NATIVE(native_ease_quad_in_out, "ease_quad_in_out", easeQuadInOut)
DEFINE_EASE_NATIVE(native_ease_cubic_in, "ease_cubic_in", easeCubicIn)
DEFINE_EASE_NATIVE(native_ease_cubic_out, "ease_cubic_out", easeCubicOut)
DEFINE_EASE_NATIVE(native_ease_cubic_in_out, "ease_cubic_in_out", easeCubicInOut)
DEFINE_EASE_NATIVE(native_ease_quart_in, "ease_quart_in", easeQuartIn)
DEFINE_EASE_NATIVE(native_ease_quart_out, "ease_quart_out", easeQuartOut)
DEFINE_EASE_NATIVE(native_ease_quart_in_out, "ease_quart_in_out", easeQuartInOut)
DEFINE_EASE_NATIVE(native_ease_quint_in, "ease_quint_in", easeQuintIn)
DEFINE_EASE_NATIVE(native_ease_quint_out, "ease_quint_out", easeQuintOut)
DEFINE_EASE_NATIVE(native_ease_quint_in_out, "ease_quint_in_out", easeQuintInOut)
DEFINE_EASE_NATIVE(native_ease_expo_in, "ease_expo_in", easeExpoIn)
DEFINE_EASE_NATIVE(native_ease_expo_out, "ease_expo_out", easeExpoOut)
DEFINE_EASE_NATIVE(native_ease_expo_in_out, "ease_expo_in_out", easeExpoInOut)
DEFINE_EASE_NATIVE(native_ease_circ_in, "ease_circ_in", easeCircIn)
DEFINE_EASE_NATIVE(native_ease_circ_out, "ease_circ_out", easeCircOut)
DEFINE_EASE_NATIVE(native_ease_circ_in_out, "ease_circ_in_out", easeCircInOut)
DEFINE_EASE_NATIVE(native_ease_back_in, "ease_back_in", easeBackIn)
DEFINE_EASE_NATIVE(native_ease_back_out, "ease_back_out", easeBackOut)
DEFINE_EASE_NATIVE(native_ease_back_in_out, "ease_back_in_out", easeBackInOut)
DEFINE_EASE_NATIVE(native_ease_elastic_in, "ease_elastic_in", easeElasticIn)
DEFINE_EASE_NATIVE(native_ease_elastic_out, "ease_elastic_out", easeElasticOut)
DEFINE_EASE_NATIVE(native_ease_elastic_in_out, "ease_elastic_in_out", easeElasticInOut)
DEFINE_EASE_NATIVE(native_ease_bounce_in, "ease_bounce_in", easeBounceIn)
DEFINE_EASE_NATIVE(native_ease_bounce_out, "ease_bounce_out", easeBounceOut)
DEFINE_EASE_NATIVE(native_ease_bounce_in_out, "ease_bounce_in_out", easeBounceInOut)

#undef DEFINE_EASE_NATIVE

void registerAll(Interpreter &vm)
{
    vm.registerNative("ease_linear", native_ease_linear, 1);
    vm.registerNative("ease_sine_in", native_ease_sine_in, 1);
    vm.registerNative("ease_sine_out", native_ease_sine_out, 1);
    vm.registerNative("ease_sine_in_out", native_ease_sine_in_out, 1);
    vm.registerNative("ease_quad_in", native_ease_quad_in, 1);
    vm.registerNative("ease_quad_out", native_ease_quad_out, 1);
    vm.registerNative("ease_quad_in_out", native_ease_quad_in_out, 1);
    vm.registerNative("ease_cubic_in", native_ease_cubic_in, 1);
    vm.registerNative("ease_cubic_out", native_ease_cubic_out, 1);
    vm.registerNative("ease_cubic_in_out", native_ease_cubic_in_out, 1);
    vm.registerNative("ease_quart_in", native_ease_quart_in, 1);
    vm.registerNative("ease_quart_out", native_ease_quart_out, 1);
    vm.registerNative("ease_quart_in_out", native_ease_quart_in_out, 1);
    vm.registerNative("ease_quint_in", native_ease_quint_in, 1);
    vm.registerNative("ease_quint_out", native_ease_quint_out, 1);
    vm.registerNative("ease_quint_in_out", native_ease_quint_in_out, 1);
    vm.registerNative("ease_expo_in", native_ease_expo_in, 1);
    vm.registerNative("ease_expo_out", native_ease_expo_out, 1);
    vm.registerNative("ease_expo_in_out", native_ease_expo_in_out, 1);
    vm.registerNative("ease_circ_in", native_ease_circ_in, 1);
    vm.registerNative("ease_circ_out", native_ease_circ_out, 1);
    vm.registerNative("ease_circ_in_out", native_ease_circ_in_out, 1);
    vm.registerNative("ease_back_in", native_ease_back_in, 1);
    vm.registerNative("ease_back_out", native_ease_back_out, 1);
    vm.registerNative("ease_back_in_out", native_ease_back_in_out, 1);
    vm.registerNative("ease_elastic_in", native_ease_elastic_in, 1);
    vm.registerNative("ease_elastic_out", native_ease_elastic_out, 1);
    vm.registerNative("ease_elastic_in_out", native_ease_elastic_in_out, 1);
    vm.registerNative("ease_bounce_in", native_ease_bounce_in, 1);
    vm.registerNative("ease_bounce_out", native_ease_bounce_out, 1);
    vm.registerNative("ease_bounce_in_out", native_ease_bounce_in_out, 1);
}
} // namespace BindingsEase
