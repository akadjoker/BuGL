
#include "interpreter.hpp"

#ifdef BU_ENABLE_MATH

#include <random>
#include <climits>
#include <random>
#include <algorithm>
#include <cmath>

class RandomGenerator
{
private:
    std::mt19937 engine;

    RandomGenerator()
    {
        std::random_device rd;
        engine.seed(rd());
    }

public:
    static RandomGenerator &instance()
    {
        static RandomGenerator inst;
        return inst;
    }

    // Permite definir uma seed fixa para determinismo
    void setSeed(unsigned int seed)
    {
        engine.seed(seed);
    }

    // [0, INT_MAX]
    int rand()
    {
        return std::uniform_int_distribution<int>(0, INT_MAX)(engine);
    }

    // [0, max]
    int rand(int max)
    {
        if (max < 0)
            return 0; // Proteção simples
        return std::uniform_int_distribution<int>(0, max)(engine);
    }

    // [min, max]
    int rand(int min, int max)
    {
        if (min > max)
            std::swap(min, max); // Proteção contra crash
        return std::uniform_int_distribution<int>(min, max)(engine);
    }

    // [0.0, 1.0]
    double randFloat()
    {
        return std::uniform_real_distribution<double>(0.0, 1.0)(engine);
    }

    // [min, max]
    double randFloat(double min, double max)
    {
        if (min > max)
            std::swap(min, max);
        return std::uniform_real_distribution<double>(min, max)(engine);
    }
};

int native_seed(Interpreter *vm, int argCount, Value *args)
{
    if (argCount == 1 && args[0].isInt())
    {
        RandomGenerator::instance().setSeed((unsigned int)args[0].asNumber());
    }
    return 0;
}

int native_rand(Interpreter *vm, int argCount, Value *args)
{

    if (argCount == 0)
    {
        vm->push(vm->makeDouble(RandomGenerator::instance().randFloat()));
        return 1;
    }
    else if (argCount == 1)
    {
        double value = args[0].asDouble();
        vm->push(vm->makeDouble(RandomGenerator::instance().randFloat(0, value)));
        return 1;
    }
    else
    {
        double min = args[0].asDouble();
        double max = args[1].asDouble();
        vm->push(vm->makeDouble(RandomGenerator::instance().randFloat(min, max)));
        return 1;
    }
    return 0;
}

int native_irand(Interpreter *vm, int argCount, Value *args)
{

    if (argCount == 0)
    {
        vm->push(vm->makeInt(RandomGenerator::instance().rand()));
        return 1;
    }
    else if (argCount == 1)
    {
        int value = args[0].asInt();
        vm->push(vm->makeInt(RandomGenerator::instance().rand(0, value)));
        return 1;
    }
    else
    {
        int min = args[0].asInt();
        int max = args[1].asInt();
        vm->push(vm->makeInt(RandomGenerator::instance().rand(min, max)));
        return 1;
    }
    return 0;
}

int native_min(Interpreter *vm, int argCount, Value *args)
{

    if (argCount != 2)
    {
        vm->runtimeError("min expects 2 arguments");
        return 0;
    }

    bool isInt = args[0].isInt() && args[1].isInt();
    if (isInt)
    {
        vm->push(vm->makeInt(std::min(args[0].asInt(), args[1].asInt())));
        return 1;
    }

    vm->push(vm->makeDouble(std::min(args[0].asNumber(), args[1].asNumber())));
    return 1;
}

int native_max(Interpreter *vm, int argCount, Value *args)
{

    if (argCount != 2)
    {
        vm->runtimeError("max expects 2 arguments");
        return 0;
    }

    bool isInt = args[0].isInt() && args[1].isInt();
    if (isInt)
    {
        vm->push(vm->makeInt(std::max(args[0].asInt(), args[1].asInt())));
        return 1;
    }

    vm->push(vm->makeDouble(std::max(args[0].asNumber(), args[1].asNumber())));
    return 1;
}

int native_clamp(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 3)
    {
        vm->runtimeError("clamp expects 3 arguments");
        return 0;
    }

    if (args[0].isInt() && args[1].isInt() && args[2].isInt())
    {
        int v = args[0].asInt();
        int lo = args[1].asInt();
        int hi = args[2].asInt();
        if (v < lo)
        {
            vm->push(vm->makeInt(lo));
            return 1;
        }
        if (v > hi)
        {
            vm->push(vm->makeInt(hi));
            return 1;
        }
        vm->push(vm->makeInt(v));
        return 1;
    }

    double v = args[0].asNumber();
    double lo = args[1].asNumber();
    double hi = args[2].asNumber();

    if (v < lo)
    {
        vm->push(vm->makeDouble(lo));
        return 1;
    }
    if (v > hi)
    {
        vm->push(vm->makeDouble(hi));
        return 1;
    }
    vm->push(vm->makeDouble(v));
    return 1;
}

// ==========================================
// INTERPOLAÇÃO & RANGES (Game Dev Essentials)
// ==========================================

// lerp(start, end, t) -> Interpolação Linear
int native_math_lerp(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 3)
    {
        vm->runtimeError("lerp expects 3 arguments");
        return 0;
    }
    double a = args[0].asNumber();
    double b = args[1].asNumber();
    double t = args[2].asNumber();
    vm->push(vm->makeDouble(a + t * (b - a)));
    return 1;
}

// catmull(p0, p1, p2, p3, t) -> Catmull-Rom interpolation (scalar)
int native_math_catmull(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 5)
    {
        vm->runtimeError("catmull expects 5 arguments (p0, p1, p2, p3, t)");
        return 0;
    }

    const double p0 = args[0].asNumber();
    const double p1 = args[1].asNumber();
    const double p2 = args[2].asNumber();
    const double p3 = args[3].asNumber();
    const double t = args[4].asNumber();

    const double t2 = t * t;
    const double t3 = t2 * t;
    const double out = 0.5 * ((2.0 * p1) +
                              (-p0 + p2) * t +
                              (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
                              (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);

    vm->push(vm->makeDouble(out));
    return 1;
}

// map(value, inMin, inMax, outMin, outMax) -> Remapeia valores
int native_math_map(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 5)
    {
        vm->runtimeError("map expects 5 arguments");
        return 0;
    }
    double x = args[0].asNumber();
    double in_min = args[1].asNumber();
    double in_max = args[2].asNumber();
    double out_min = args[3].asNumber();
    double out_max = args[4].asNumber();

    vm->push(vm->makeDouble((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min));
    return 1;
}

// sign(x) -> Retorna -1, 0, ou 1
int native_math_sign(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1)
    {
        vm->runtimeError("sign expects 1 argument");
        return 0;
    }

    double val = args[0].asNumber();
    if (val > 0)
    {
        vm->push(vm->makeDouble(1));
        return 1;
    }
    if (val < 0)
    {
        vm->push(vm->makeDouble(-1));
        return 1;
    }
    vm->push(vm->makeDouble(0));
    return 1;
}

// ==========================================
// GEOMETRIA & DISTÂNCIA
// ==========================================

// hypot(dx, dy) -> sqrt(x*x + y*y) (Seguro contra overflow)
int native_math_hypot(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 2)
    {
        vm->runtimeError("hypot expects 2 arguments");
        return 0;
    }
    vm->push(vm->makeDouble(std::hypot(args[0].asNumber(), args[1].asNumber())));
    return 1;
}

// ==========================================
// LOGARITMOS EXTRA & HIPERBÓLICAS
// ==========================================

// log10(x)
int native_math_log10(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1)
        return 0;
    vm->push(vm->makeDouble(std::log10(args[0].asNumber())));
    return 1;
}

// log2(x)
int native_math_log2(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1)
        return 0;
    vm->push(vm->makeDouble(std::log2(args[0].asNumber())));
    return 1;
}

// sinh(x)
int native_math_sinh(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1)
        return 0;
    vm->push(vm->makeDouble(std::sinh(args[0].asNumber())));
    return 1;
}

// cosh(x)
int native_math_cosh(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1)
        return 0;
    vm->push(vm->makeDouble(std::cosh(args[0].asNumber())));
    return 1;
}

// tanh(x)
int native_math_tanh(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1)
        return 0;
    vm->push(vm->makeDouble(std::tanh(args[0].asNumber())));
    return 1;
}

static double CLAMP(double x, double min, double max) { return x < min ? min : x > max ? max
                                                                                       : x; }

int native_math_smoothstep(Interpreter *vm, int argCount, Value *args)
{
    // Suporta 1 argumento (t normalizado 0..1) ou 3 argumentos (range GLSL)
    double t, edge0 = 0.0, edge1 = 1.0;

    if (argCount == 1)
    {
        t = args[0].asNumber();
    }
    else if (argCount == 3)
    {
        edge0 = args[0].asNumber();
        edge1 = args[1].asNumber();
        t = args[2].asNumber();
    }
    else
    {
        vm->runtimeError("smoothstep expects 1 or 3 arguments");
        return 0;
    }

    // Clamp e normalização
    t = CLAMP((t - edge0) / (edge1 - edge0), 0.0, 1.0);

    // Fórmula: t * t * (3 - 2 * t)
    vm->push(vm->makeDouble(t * t * (3.0 - 2.0 * t)));
    return 1;
}

// smootherstep(edge0, edge1, x)
// Versão do Ken Perlin, transição ainda mais suave que o smoothstep.
int native_math_smootherstep(Interpreter *vm, int argCount, Value *args)
{
    double t, edge0 = 0.0, edge1 = 1.0;

    if (argCount == 1)
    {
        t = args[0].asNumber();
    }
    else if (argCount == 3)
    {
        edge0 = args[0].asNumber();
        edge1 = args[1].asNumber();
        t = args[2].asNumber();
    }
    else
    {
        vm->runtimeError("smootherstep expects 1 or 3 arguments");
        return 0;
    }

    t = CLAMP((t - edge0) / (edge1 - edge0), 0.0, 1.0);

    // Fórmula: t * t * t * (t * (t * 6 - 15) + 10)
    vm->push(vm->makeDouble(t * t * t * (t * (t * 6.0 - 15.0) + 10.0)));
    return 1;
}

double hermite(double value1, double tangent1, double value2, double tangent2, double amount)
{
    double v1 = value1;
    double v2 = value2;
    double t1 = tangent1;
    double t2 = tangent2;
    double s = amount;
    double result;
    double sCubed = s * s * s;
    double sSquared = s * s;

    if (amount == 0)
        result = value1;
    else if (amount == 1)
        result = value2;
    else
        result = (2 * v1 - 2 * v2 + t2 + t1) * sCubed +
                 (3 * v2 - 3 * v1 - 2 * t1 - t2) * sSquared +
                 t1 * s +
                 v1;
    return result;
}

float repeat(double t, double length)
{
    return CLAMP(t - std::floor(t / length) * length, 0.0f, length);
}
double ping_pong(double t, double length)
{
    t = repeat(t, length * 2.0f);
    return length - std::abs(t - length);
}

int native_math_hermite(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 5)
    {
        vm->runtimeError("hermite expects 5 arguments");
        return 0;
    }
    vm->push(vm->makeDouble(hermite(args[0].asNumber(), args[1].asNumber(), args[2].asNumber(), args[3].asNumber(), args[4].asNumber())));
    return 1;
}

int native_math_repeat(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 2)
    {
        vm->runtimeError("repeat expects 2 arguments");
        return 0;
    }
    vm->push(vm->makeDouble(repeat(args[0].asNumber(), args[1].asNumber())));
    return 1;
}

int native_math_ping_pong(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 2)
    {
        vm->runtimeError("ping_pong expects 2 arguments");
        return 0;
    }
    vm->push(vm->makeDouble(ping_pong(args[0].asNumber(), args[1].asNumber())));
    return 1;
}

// abs(value) -> absolute value
 int native_abs(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isNumber())
    {
        Error("abs expects 1 number argument");
        vm->pushDouble(0);
        return 1;
    }
    vm->pushDouble(fabs(args[0].asNumber()));
    return 1;
}

void Interpreter::registerMath()
{

    addModule("math")
        .addDouble("PI", 3.14159265358979)
        .addDouble("E", 2.71828182845905)
        .addDouble("TAU", 6.28318530717958647692)
        .addFloat("SQRT2", 1.41421356f)
        .addInt("MIN_INT", -2147483648)
        .addInt("MAX_INT", 2147483647)

        // Utils de Jogos/Lógica
        .addFunction("lerp", native_math_lerp, 3)
        .addFunction("catmull", native_math_catmull, 5)
        .addFunction("map", native_math_map, 5)
        .addFunction("sign", native_math_sign, 1)
        .addFunction("hypot", native_math_hypot, 2)

        // Logs específicos (o Opcode LOG geralmente é base e/ln)
        .addFunction("log10", native_math_log10, 1)
        .addFunction("log2", native_math_log2, 1)

        // Hiperbólicas
        .addFunction("sinh", native_math_sinh, 1)
        .addFunction("cosh", native_math_cosh, 1)
        .addFunction("tanh", native_math_tanh, 1)

        // Funções de transição
        .addFunction("smoothstep", native_math_smoothstep, -1)
        .addFunction("smootherstep", native_math_smootherstep, -1)
        .addFunction("hermite", native_math_hermite, 5)
        .addFunction("repeat", native_math_repeat, 2)
        .addFunction("ping_pong", native_math_ping_pong, 2)

        .addFunction("abs", native_abs, 1)
        .addFunction("clamp", native_clamp, 3)
        .addFunction("min", native_min, 2)
        .addFunction("max", native_max, 2)
        .addFunction("seed", native_seed, 1)
        .addFunction("rand", native_rand, -1)
        .addFunction("irand", native_irand, -1);
}

#endif
