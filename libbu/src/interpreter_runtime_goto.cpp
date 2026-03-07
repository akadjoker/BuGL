/**
 * @file interpreter_runtime_goto.cpp
 * @brief VM runtime interpreter using computed goto for opcode dispatch
 *
 * Implements the core execution engine for the BuLang virtual machine using
 * computed goto optimization for fast opcode dispatch. Handles:
 * - Stack-based operations (push, pop, duplicate, swap)
 * - Arithmetic and bitwise operations with type coercion
 * - Variable access (local, global, private)
 * - Control flow (jumps, loops, gosub/return)
 * - Function calls and returns with frame management
 * - Object-oriented features (classes, methods, properties, inheritance)
 * - Exception handling (try-catch-finally with proper cleanup)
 * - Closure and upvalue management
 * - Collection operations (arrays, maps, buffers)
 * - String manipulation methods
 * - Mathematical functions (trigonometric, logarithmic, etc.)
 * - Process/ProcessExec management and concurrency primitives
 * - Native class/struct integration
 * - Module function calls
 *
 * The interpreter uses a dispatch table with computed goto for O(1) opcode
 * routing, avoiding switch statement overhead. Each opcode is implemented
 * as a labeled block that reads operands, performs the operation, and
 * dispatches to the next instruction.
 *
 * @note Compiled only when USE_COMPUTED_GOTO is 1
 * @note Uses macro-based stack manipulation for performance
 * @note Manages call frames for nested function invocations
 * @note Handles both user-defined and native functions/classes
 *
 * @param fiber The current fiber to execute
 * @param process The parent process context
 * @return ProcessResult containing status, instruction count, and optional metrics
 */
#include "interpreter.hpp"
#include "pool.hpp"
#include "opcode.hpp"
#include "debug.hpp"
#include "platform.hpp"
#include <cmath> // std::fmod
#include <new>
#include <ctime>

#if USE_COMPUTED_GOTO

extern size_t get_type_size(BufferType type);

#define DEBUG_TRACE_EXECUTION 0 // 1 = ativa, 0 = desativa
#define DEBUG_TRACE_STACK 0     // 1 = mostra stack, 0 = esconde

inline bool toNumberPair(const Value &a, const Value &b, double &da, double &db)
{
    if (!(a.isInt() || a.isDouble()))
        return false;
    if (!(b.isInt() || b.isDouble()))
        return false;

    da = a.isInt() ? static_cast<double>(a.asInt()) : a.asDouble();
    db = b.isInt() ? static_cast<double>(b.asInt()) : b.asDouble();
    return true;
}

inline  const char *getValueTypeName(const Value &v)
{
    switch (v.type)
    {
    case ValueType::NIL:
        return "nil";
    case ValueType::BOOL:
        return "bool";
    case ValueType::CHAR:
        return "char";
    case ValueType::BYTE:
        return "byte";
    case ValueType::INT:
        return "int";
    case ValueType::UINT:
        return "uint";
    case ValueType::LONG:
        return "long";
    case ValueType::ULONG:
        return "ulong";
    case ValueType::FLOAT:
        return "float";
    case ValueType::DOUBLE:
        return "double";
    case ValueType::STRING:
        return "string";
    case ValueType::ARRAY:
        return "array";
    case ValueType::MAP:
        return "map";
    case ValueType::BUFFER:
        return "buffer";
    case ValueType::STRUCT:
        return "struct";
    case ValueType::STRUCTINSTANCE:
        return "struct instance";
    case ValueType::FUNCTION:
        return "function";
    case ValueType::NATIVE:
        return "native function";
    case ValueType::NATIVECLASS:
        return "native class";
    case ValueType::NATIVECLASSINSTANCE:
        return "native class instance";
    case ValueType::NATIVESTRUCT:
        return "native struct";
    case ValueType::NATIVESTRUCTINSTANCE:
        return "native struct instance";
    case ValueType::CLASS:
        return "class";
    case ValueType::CLASSINSTANCE:
        return "class instance";
    case ValueType::PROCESS:
        return "process";
    case ValueType::POINTER:
        return "pointer";
    case ValueType::MODULEREFERENCE:
        return "module reference";
    case ValueType::CLOSURE:
        return "closure";
    default:
        return "unknown";
    }
}

ProcessResult Interpreter::run_process(Process *process)
{
    ProcessExec *fiber = process;
    currentProcess = process;

    CallFrame *frame;
    Value *stackStart;
    uint8 *ip;
    Function *func;

    
// Macros
#define DROP() (fiber->stackTop--)
#define PEEK() (*(fiber->stackTop - 1))
#define PEEK2() (*(fiber->stackTop - 2))
#define POP() (*(--fiber->stackTop))
#define PUSH(value) (*fiber->stackTop++ = value)
#define NPEEK(n) (fiber->stackTop[-1 - (n)])
#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (func->chunk->constants[READ_SHORT()])

#define BINARY_OP_PREP()           \
    Value b = fiber->stackTop[-1]; \
    Value a = fiber->stackTop[-2]; \
    fiber->stackTop -= 2

#define STORE_FRAME() frame->ip = ip

#define THROW_RUNTIME_ERROR(fmt, ...)                                \
    do                                                               \
    {                                                                \
        STORE_FRAME();                                               \
        char msgBuffer[256];                                         \
        snprintf(msgBuffer, sizeof(msgBuffer), fmt, ##__VA_ARGS__);  \
                                                                     \
        Value errorVal = makeString(msgBuffer);                      \
                                                                     \
        if (throwException(errorVal))                                \
        {                                                            \
            ip = fiber->ip;                                          \
            DISPATCH();                                              \
        }                                                            \
        else                                                         \
        {                                                            \
            runtimeError("%s", msgBuffer);                           \
            return {ProcessResult::PROCESS_DONE, 0}; \
        }                                                            \
    } while (0)

#define LOAD_FRAME()                                   \
    do                                                 \
    {                                                  \
        assert(fiber->frameCount > 0);                 \
        frame = &fiber->frames[fiber->frameCount - 1]; \
        stackStart = frame->slots;                     \
        ip = frame->ip;                                \
        func = frame->func;                            \
    } while (false)

    static const void *dispatch_table[] = {
        // Literals (0-3)
        &&op_constant,
        &&op_nil,
        &&op_true,
        &&op_false,

        // Stack (4-7)
        &&op_pop,
        &&op_halt,
        &&op_not,
        &&op_dup,

        // Arithmetic (8-13)
        &&op_add,
        &&op_subtract,
        &&op_multiply,
        &&op_divide,
        &&op_negate,
        &&op_modulo,

        // Bitwise (14-19)
        &&op_bitwise_and,
        &&op_bitwise_or,
        &&op_bitwise_xor,
        &&op_bitwise_not,
        &&op_shift_left,
        &&op_shift_right,

        // Comparisons (20-25)
        &&op_equal,
        &&op_not_equal,
        &&op_greater,
        &&op_greater_equal,
        &&op_less,
        &&op_less_equal,

        // Variables (26-32)
        &&op_get_local,
        &&op_set_local,
        &&op_get_global,
        &&op_set_global,
        &&op_define_global,
        &&op_get_private,
        &&op_set_private,

        // Control flow (33-37)
        &&op_jump,
        &&op_jump_if_false,
        &&op_loop,
        &&op_gosub,
        &&op_return_sub,

        // Functions (38-43)
        &&op_call,
        &&op_return,
        &&op_array_push,
        &&op_legacy_fiber_opcode,
        &&op_frame,
        &&op_exit,

        // Collections (44-45)
        &&op_define_array,
        &&op_define_map,

        // Properties (46-49)
        &&op_get_property,
        &&op_set_property,
        &&op_get_index,
        &&op_set_index,

        // Methods (50-51)
        &&op_invoke,
        &&op_super_invoke,

        // I/O (52)
        &&op_print,
        // inter functions
        &&op_len,
        // forech
        &&op_iter_next,
        &&op_iter_value,
        &&op_copy2,
        &&op_swap,
        &&op_discard,
        &&op_try,
        &&op_pop_try,
        &&op_throw,
        &&op_enter_catch,
        &&op_enter_finally,
        &&op_exit_finally,

        // --- MATH UNARY
        &&op_sin,   // 65
        &&op_cos,   // 66
        &&op_tan,   // 67
        &&op_asin,  // 68
        &&op_acos,  // 69
        &&op_atan,  // 70
        &&op_sqrt,  // 71
        &&op_abs,   // 72
        &&op_log,   // 73
        &&op_floor, // 74
        &&op_ceil,  // 75
        &&op_deg,   // 76
        &&op_rad,   // 77
        &&op_exp,   // 78

        // --- MATH BINARY (Aqui estava o erro, faltavam estes dois) ---
        &&op_atan2, // 79
        &&op_pow,   // 80

        // --- UTILS ---
        &&op_clock,      // 81
        &&op_new_buffer, // 82
        &&op_free,       // 83
        &&op_closure,
        &&op_get_upvalue,
        &&op_set_upvalue,
        &&op_close_upvalue,

        // Multi-return (88)
        &&op_return_n,

        // Type reference (89)
        &&op_type,

        // Process utilities (90-91)
        &&op_proc,
        &&op_get_id,
    };

#define SAFE_CALL_NATIVE(fiber, argCount, callFunc)                                    \
    do                                                                                 \
    {                                                                                  \
        /* 1. Calcular o OFFSET (Índice seguro) */                                     \
        size_t _slot = ((fiber)->stackTop - (fiber)->stack) - (argCount) - 1;          \
                                                                                       \
        /* 2. Definir _args para ser usado dentro da 'callFunc' */                     \
        Value *_args = &(fiber)->stack[_slot + 1];                                     \
                                                                                       \
        /* 3. CHAMADA (Aqui o _args é passado implicitamente na expressão callFunc) */ \
        int _rets = (callFunc);                                                        \
                                                                                       \
        /* 4. RECALCULAR DESTINO (Seguro contra realloc) */                            \
        Value *_dest = &(fiber)->stack[_slot];                                         \
                                                                                       \
        /* 5. Processar Retornos */                                                    \
        if (_rets > 0)                                                                 \
        {                                                                              \
            Value *_src = (fiber)->stackTop - _rets;                                   \
            if (_src != _dest)                                                         \
            {                                                                          \
                /* memmove é necessário se houver overlap, copy se não houver */       \
                std::memmove(_dest, _src, _rets * sizeof(Value));                      \
            }                                                                          \
            (fiber)->stackTop = _dest + _rets;                                         \
        }                                                                              \
        else                                                                           \
        {                                                                              \
            /* Retornar nil se for void */                                             \
            *_dest = makeNil();                                                        \
            (fiber)->stackTop = _dest + 1;                                             \
        }                                                                              \
    } while (0)

#define DISPATCH()                         \
    do                                     \
    {                                      \
                goto *dispatch_table[READ_BYTE()]; \
    } while (0)

#define ENTER_CALL_FRAME_DISPATCH(_targetFunc, _closure, _argc, _overflowMsg) \
    do                                                                          \
    {                                                                           \
        if (fiber->frameCount >= FRAMES_MAX)                                   \
        {                                                                       \
            runtimeError(_overflowMsg);                                         \
            return {ProcessResult::PROCESS_DONE, 0};                            \
        }                                                                       \
        CallFrame *newFrame = &fiber->frames[fiber->frameCount++];             \
        newFrame->func = (_targetFunc);                                         \
        newFrame->closure = (_closure);                                         \
        newFrame->ip = (_targetFunc)->chunk->code;                              \
        newFrame->slots = fiber->stackTop - (_argc) - 1;                        \
        frame = newFrame;                                                       \
        stackStart = newFrame->slots;                                           \
        ip = newFrame->ip;                                                      \
        func = newFrame->func;                                                  \
        DISPATCH();                                                             \
    } while (0)

#define ENTER_CALL_FRAME_DISPATCH_STORE(_targetFunc, _closure, _argc, _overflowMsg) \
    do                                                                                \
    {                                                                                 \
        STORE_FRAME();                                                                 \
        ENTER_CALL_FRAME_DISPATCH(_targetFunc, _closure, _argc, _overflowMsg);        \
    } while (0)

    LOAD_FRAME();

    DISPATCH();

op_constant:
{
    const Value &constant = READ_CONSTANT();
    PUSH(constant);
    DISPATCH();
}

op_nil:
{
    PUSH(makeNil());
    DISPATCH();
}

op_true:
{
    PUSH(makeBool(true));
    DISPATCH();
}

op_false:
{
    PUSH(makeBool(false));
    DISPATCH();
}

op_dup:
{
    PUSH(PEEK());
    DISPATCH();
}

    // ========== STACK MANIPULATION ==========

op_pop:
{
    POP();
    DISPATCH();
}

op_halt:
{
    return {ProcessResult::PROCESS_DONE, 0};
}

    // ========== VARIABLES ==========

op_get_local:
{
    uint8 slot = READ_BYTE();

    PUSH(stackStart[slot]);
    DISPATCH();
}

op_set_local:
{
    uint8 slot = READ_BYTE();
    stackStart[slot] = PEEK();
    DISPATCH();
}

op_get_private:
{
    uint8 index = READ_BYTE();
    PUSH(process->privates[index]);
    DISPATCH();
}

op_set_private:
{
    uint8 index = READ_BYTE();
    process->privates[index] = PEEK();
    DISPATCH();
}

op_get_global:
{
    // OPTIMIZATION: Direct array access using index instead of hash lookup
    uint16 index = READ_SHORT();
    Value value = globalsArray[index];

    PUSH(value);
    DISPATCH();
}

op_set_global:
{
    // OPTIMIZATION: Direct array access using index instead of hash lookup
    uint16 index = READ_SHORT();
    globalsArray[index] = PEEK();
    DISPATCH();
}

op_define_global:
{
    // OPTIMIZATION: Direct array access using index instead of hash lookup
    uint16 index = READ_SHORT();
    globalsArray[index] = POP();
    DISPATCH();
}

    // ========== ARITHMETIC ==========

// ============================================
// OP_ADD
// ============================================
op_add:
{
    BINARY_OP_PREP();

    // ---------------------------------------------------------
    // 1. CONCATENAÇÃO (String à Esquerda)
    // Ex: "Pontos: " + 100
    // ---------------------------------------------------------
    if (a.isString())
    {
        if (b.isString())
        {
            PUSH(makeString(stringPool.concat(a.asString(), b.asString())));
            DISPATCH();
        }
        else if (b.isInt())
        {
            String *right = stringPool.toString(b.asInt());
            PUSH(makeString(stringPool.concat(a.asString(), right)));
            DISPATCH();
        }
        else if (b.isUInt())
        {
            String *right = stringPool.toString(b.asUInt());
            PUSH(makeString(stringPool.concat(a.asString(), right)));
            DISPATCH();
        }
        else if (b.isDouble())
        {
            String *right = stringPool.toString(b.asDouble());
            PUSH(makeString(stringPool.concat(a.asString(), right)));
            DISPATCH();
        }
        else if (b.isBool())
        {
            String *right = stringPool.toString(b.asBool());
            PUSH(makeString(stringPool.concat(a.asString(), right)));
            DISPATCH();
        }
        else if (b.isNil())
        {
            String *right = createString("nil");
            PUSH(makeString(stringPool.concat(a.asString(), right)));
            DISPATCH();
        }
        else if (b.isByte())
        {
            String *right = stringPool.toString(b.asByte());
            PUSH(makeString(stringPool.concat(a.asString(), right)));
            DISPATCH();
        }
    }

    // ---------------------------------------------------------
    // 2. CONCATENAÇÃO REVERSA
    // Ex: 100 + " pontos"

    // ---------------------------------------------------------
    else if (b.isString())
    {
        if (a.isInt())
        {
            String *left = stringPool.toString(a.asInt());
            PUSH(makeString(stringPool.concat(left, b.asString())));
            DISPATCH();
        }
        else if (a.isDouble())
        {
            String *left = stringPool.toString(a.asDouble());
            PUSH(makeString(stringPool.concat(left, b.asString())));
            DISPATCH();
        }
        else if (a.isUInt())
        {
            String *left = stringPool.toString(a.asUInt());
            PUSH(makeString(stringPool.concat(left, b.asString())));
            DISPATCH();
        }
        else if (a.isBool())
        {
            String *left = stringPool.toString(a.asBool());
            PUSH(makeString(stringPool.concat(left, b.asString())));
            DISPATCH();
        }
        else if (a.isNil())
        {
            String *left = createString("nil");
            PUSH(makeString(stringPool.concat(left, b.asString())));
            DISPATCH();
        }
        else if (a.isByte())
        {
            String *left = stringPool.toString(a.asByte());
            PUSH(makeString(stringPool.concat(left, b.asString())));
            DISPATCH();
        }
    }

    else if (a.isNumber() && b.isNumber())
    {
        // Caminho rápido: Inteiros
        if (a.isInt() && b.isInt())
        {
            PUSH(makeInt(a.asInt() + b.asInt()));
        }
        // Caminho misto: Converte tudo para double
        else
        {
            double da = a.isInt() ? (double)a.asInt() : a.asDouble();
            double db = b.isInt() ? (double)b.asInt() : b.asDouble();
            PUSH(makeDouble(da + db));
        }
        DISPATCH();
    }

    THROW_RUNTIME_ERROR("Cannot apply '+' to %s and %s", getValueTypeName(a), getValueTypeName(b));
}
// ============================================
// OP_SUBTRACT
// ============================================
op_subtract:
{
    BINARY_OP_PREP();

    if (a.isNumber() && b.isNumber())
    {
        if (a.isInt() && b.isInt())
        {
            PUSH(makeInt(a.asInt() - b.asInt()));
        }
        else
        {
            double da = a.isInt() ? (double)a.asInt() : a.asDouble();
            double db = b.isInt() ? (double)b.asInt() : b.asDouble();
            PUSH(makeDouble(da - db));
        }
        DISPATCH();
    }
    else if (a.isBool() && b.isNumber())
    {
        double da = a.asBool() ? 1.0 : 0.0;
        double db = b.isInt() ? (double)b.asInt() : b.asDouble();
        PUSH(makeDouble(da - db));
        DISPATCH();
    }
    else if (a.isNumber() && b.isBool())
    {
        double da = a.isInt() ? (double)a.asInt() : a.asDouble();
        double db = b.asBool() ? 1.0 : 0.0;
        PUSH(makeDouble(da - db));
        DISPATCH();
    }
    else if (a.isBool() && b.isBool())
    {
        double da = a.asBool() ? 1.0 : 0.0;
        double db = b.asBool() ? 1.0 : 0.0;
        PUSH(makeDouble(da - db));
        DISPATCH();
    }

    THROW_RUNTIME_ERROR("Cannot apply '-' to %s and %s", getValueTypeName(a), getValueTypeName(b));
}

// ============================================
// OP_MULTIPLY
// ============================================
op_multiply:
{
    BINARY_OP_PREP();

    if (a.isNumber() && b.isNumber())
    {
        if (a.isInt() && b.isInt())
        {
            PUSH(makeInt(a.asInt() * b.asInt()));
        }
        else
        {
            double da = a.isInt() ? (double)a.asInt() : a.asDouble();
            double db = b.isInt() ? (double)b.asInt() : b.asDouble();
            PUSH(makeDouble(da * db));
        }
        DISPATCH();
    }

    THROW_RUNTIME_ERROR("Cannot apply '*' to %s and %s", getValueTypeName(a), getValueTypeName(b));
}

// ============================================
// OP_DIVIDE
// ============================================
op_divide:
{
    BINARY_OP_PREP();
#define THROW_DIV_ZERO()                                             \
    do                                                               \
    {                                                                \
        STORE_FRAME();                                               \
        Value error = makeString("Division by zero");                \
                                                                     \
        if (throwException(error))                                   \
        {                                                            \
            ip = fiber->ip;                                          \
            DISPATCH();                                              \
        }                                                            \
        else                                                         \
        {                                                            \
            runtimeError("Division by zero");                        \
            return {ProcessResult::PROCESS_DONE, 0}; \
        }                                                            \
    } while (0)

    if (a.isInt() && b.isInt())
    {
        int ib = b.asInt();
        if (ib == 0)
        {
            THROW_DIV_ZERO();
        }

        int ia = a.asInt();
        if (ia % ib == 0)
        {
            PUSH(makeInt(ia / ib));
        }
        else
        {
            PUSH(makeDouble((double)ia / ib));
        }
        DISPATCH();
    }
    else if (a.isDouble() && b.isInt())
    {
        int ib = b.asInt();
        if (ib == 0)
        {
            THROW_DIV_ZERO();
        }

        PUSH(makeDouble(a.asDouble() / ib));
        DISPATCH();
    }
    else if (a.isInt() && b.isDouble())
    {
        double db = b.asDouble();
        if (db == 0.0)
        {
            THROW_DIV_ZERO();
        }

        int ia = a.asInt();
        if (fmod(ia, db) == 0)
        {
            PUSH(makeInt(ia / db));
        }
        else
        {
            PUSH(makeDouble((double)ia / db));
        }
        DISPATCH();
    }
    else if (a.isDouble() && b.isDouble())
    {
        double db = b.asDouble();
        if (db == 0.0)
        {
            THROW_DIV_ZERO();
        }

        PUSH(makeDouble(a.asDouble() / db));
        DISPATCH();
    }

    STORE_FRAME();
    runtimeError("Cannot apply '/' to %s and %s", getValueTypeName(a), getValueTypeName(b));
    return {ProcessResult::PROCESS_DONE, 0};

#undef THROW_DIV_ZERO
}

// ============================================
// OP_MODULO
// ============================================
op_modulo:
{
    BINARY_OP_PREP();

    if (!a.isNumber() || !b.isNumber())
    {
        STORE_FRAME();
        runtimeError("Cannot apply '%%' to %s and %s", getValueTypeName(a), getValueTypeName(b));
        return {ProcessResult::PROCESS_DONE, 0};
    }

#define THROW_MOD_ZERO()                                             \
    do                                                               \
    {                                                                \
        STORE_FRAME();                                               \
        Value error = makeString("Modulo by zero");                  \
        if (throwException(error))                                   \
        {                                                            \
            ip = fiber->ip;                                          \
            DISPATCH();                                              \
        }                                                            \
        else                                                         \
        {                                                            \
            runtimeError("Modulo by zero");                          \
            return {ProcessResult::PROCESS_DONE, 0}; \
        }                                                            \
    } while (0)

    if (a.isInt() && b.isInt())
    {
        if (b.asInt() == 0)
            THROW_MOD_ZERO();

        PUSH(makeInt(a.asInt() % b.asInt()));
        DISPATCH();
    }
    double da = a.isInt() ? (double)a.asInt() : a.asDouble();
    double db = b.isInt() ? (double)b.asInt() : b.asDouble();

    if (db == 0.0)
        THROW_MOD_ZERO();

    PUSH(makeDouble(fmod(da, db)));

#undef THROW_MOD_ZERO
    DISPATCH();
}

    //======== LOGICAL =====

op_negate:
{
    Value a = POP();
    if (a.isInt())
    {
        PUSH(makeInt(-a.asInt()));
    }
    else if (a.isDouble())
    {
        PUSH(makeDouble(-a.asDouble()));
    }
    else if (a.isBool())
    {
        PUSH(makeBool(!a.asBool()));
    }
    else
    {
        THROW_RUNTIME_ERROR("Operand 'NEGATE' must be a number");
    }
    DISPATCH();
}

op_equal:
{
    BINARY_OP_PREP();
    PUSH(makeBool(valuesEqual(a, b)));
    DISPATCH();
}

op_not:
{
    Value v = POP();
    PUSH(makeBool(!isTruthy(v)));
    DISPATCH();
}

op_not_equal:
{
    BINARY_OP_PREP();
    PUSH(makeBool(!valuesEqual(a, b)));
    DISPATCH();
}

op_greater:
{
    BINARY_OP_PREP();

    double da, db;
    if (!toNumberPair(a, b, da, db))
    {
        THROW_RUNTIME_ERROR("Operands '>' must be numbers");
    }

    PUSH(makeBool(da > db));
    DISPATCH();
}

op_greater_equal:
{
    BINARY_OP_PREP();

    double da, db;
    if (!toNumberPair(a, b, da, db))
    {
        THROW_RUNTIME_ERROR("Operands '>=' must be numbers");
    }
    PUSH(makeBool(da >= db));
    DISPATCH();
}

op_less:
{
    BINARY_OP_PREP();

    double da, db;
    if (!toNumberPair(a, b, da, db))
    {
        printValueNl(a);
        printValueNl(b);
        THROW_RUNTIME_ERROR("Operands '<' must be numbers");
    }

    PUSH(makeBool(da < db));
    DISPATCH();
}

op_less_equal:
{
    BINARY_OP_PREP();
    double da, db;
    if (!toNumberPair(a, b, da, db))
    {
        THROW_RUNTIME_ERROR("Operands  '<=' must be numbers");
    }
    PUSH(makeBool(da <= db));
    DISPATCH();
}

    // ======= BITWISE =====

op_bitwise_and:
{
    BINARY_OP_PREP();
    if (!a.isInt() || !b.isInt())
    {
        THROW_RUNTIME_ERROR("Bitwise AND requires integers");
    }
    PUSH(makeInt(a.asInt() & b.asInt()));
    DISPATCH();
}

op_bitwise_or:
{
    BINARY_OP_PREP();
    if (!a.isInt() || !b.isInt())
    {
        THROW_RUNTIME_ERROR("Bitwise OR requires integers");
    }
    PUSH(makeInt(a.asInt() | b.asInt()));
    DISPATCH();
}

op_bitwise_xor:
{
    BINARY_OP_PREP();
    if (!a.isInt() || !b.isInt())
    {
        THROW_RUNTIME_ERROR("Bitwise XOR requires integers");
    }
    PUSH(makeInt(a.asInt() ^ b.asInt()));
    DISPATCH();
}

op_bitwise_not:
{
    Value a = POP();
    if (!a.isInt())
    {
        THROW_RUNTIME_ERROR("Bitwise NOT requires integer");
    }
    PUSH(makeInt(~a.asInt()));
    DISPATCH();
}

op_shift_left:
{
    BINARY_OP_PREP();
    if (!a.isInt() || !b.isInt())
    {
        THROW_RUNTIME_ERROR("Shift left requires integers");
    }
    PUSH(makeInt(a.asInt() << b.asInt()));
    DISPATCH();
}

op_shift_right:
{
    BINARY_OP_PREP();
    if (!a.isInt() || !b.isInt())
    {
        THROW_RUNTIME_ERROR("Shift right requires integers");
    }
    PUSH(makeInt(a.asInt() >> b.asInt()));
    DISPATCH();
}

    // ========== CONTROL FLOW ==========

op_jump:
{
    uint16 offset = READ_SHORT();
    ip += offset;
    DISPATCH();
}

op_jump_if_false:
{
    uint16 offset = READ_SHORT();
    if (isFalsey(PEEK()))
        ip += offset;
    DISPATCH();
}

op_loop:
{
    uint16 offset = READ_SHORT();

    ip -= offset;

    DISPATCH();
}

    // ========== FUNCTIONS ==========

op_call:
{
    uint8 argCount = READ_BYTE();

    STORE_FRAME();

    Value callee = NPEEK(argCount);

    // ========================================
    // PATH 1: FUNCTION
    // ========================================
    if (callee.isFunction())
    {
        int index = callee.asFunctionId();
        Function *targetFunc = functions[index];

        if (!targetFunc)
        {
            runtimeError("Invalid function");
            return {ProcessResult::PROCESS_DONE, 0};
        }

        if (argCount != targetFunc->arity)
        {
            runtimeError("Function %s expected %d arguments but got %d",
                         targetFunc->name->chars(), targetFunc->arity, argCount);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        ENTER_CALL_FRAME_DISPATCH(targetFunc, nullptr, argCount, "Stack overflow");
    }

    // ========================================
    // PATH 2: NATIVE
    // ========================================
    else if (callee.isNative())
    {
        int index = callee.asNativeId();
        NativeDef nativeFunc = natives[index];

        if (nativeFunc.arity != -1 && argCount != nativeFunc.arity)
        {
            runtimeError("Function %s expected %d arguments but got %d",
                         nativeFunc.name->chars(), nativeFunc.arity, argCount);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        SAFE_CALL_NATIVE(fiber, argCount, nativeFunc.func(this, argCount, _args));

        DISPATCH();
    }

    // ========================================
    // PATH 2.5: NATIVE PROCESS
    // ========================================
    else if (callee.isNativeProcess())
    {
        int index = callee.asNativeProcessId();
        NativeProcessDef blueprint = nativeProcesses[index];

        if (blueprint.arity != -1 && argCount != blueprint.arity)
        {
            runtimeError("Function process expected %d arguments but got %d",
                         blueprint.arity, argCount);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        SAFE_CALL_NATIVE(fiber, argCount, blueprint.func(this, currentProcess, argCount, _args));
        DISPATCH();
    }

    // ========================================
    // PATH 3: PROCESS
    // ========================================
    else if (callee.isProcess())
    {
        int index = callee.asProcessId();
        ProcessDef *blueprint = processes[index];

        if (!blueprint)
        {
            runtimeError("Invalid process");
            return {ProcessResult::PROCESS_DONE, 0};
        }

        Function *processFunc = blueprint->frames[0].func;

        if (argCount != processFunc->arity)
        {
            runtimeError("Process expected %d arguments but got %d",
                         processFunc->arity, argCount);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        // SPAWN - clona blueprint
        Process *instance = spawnProcess(blueprint);

        // Se tem argumentos, inicializa locals da fiber
        if (argCount > 0)
        {
            ProcessExec *procFiber = instance;
            int localSlot = 0;

            for (int i = 0; i < argCount; i++)
            {
                Value arg = fiber->stackTop[-(argCount - i)];

                if (i < (int)blueprint->argsNames.size() && blueprint->argsNames[i] != 255)
                {
                    // Arg mapeia para um private (x, y, etc.) - copia direto
                    instance->privates[blueprint->argsNames[i]] = arg;
                }
                else
                {
                    // Arg é um local normal
                    procFiber->stack[localSlot] = arg;
                    localSlot++;
                }
            }

            procFiber->stackTop = procFiber->stack + localSlot;
        }

        // Remove callee + args da stack atual
        fiber->stackTop -= (argCount + 1);

        instance->privates[(int)PrivateIndex::ID] = makeInt(instance->id);
        instance->privates[(int)PrivateIndex::FATHER] = makeProcessInstance(process);

        if (hooks.onCreate)
        {
            hooks.onCreate(this,instance);
        }
        // Push process instance directly
        PUSH(makeProcessInstance(instance));

        //  Não criou frame no current fiber!
        DISPATCH();
    }

    // ========================================
    // PATH 4: STRUCT
    // ========================================
    else if (callee.isStruct())
    {
        int index = callee.as.integer;
        StructDef *def = structs[index];

        if (argCount > def->argCount)
        {
            runtimeError("Struct '%s' expects at most %zu arguments, got %d",
                         def->name->chars(), def->argCount, argCount);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        Value value = makeStructInstance();
        StructInstance *instance = value.as.sInstance;
        instance->def = def;

        instance->values.reserve(def->argCount);
        Value *args = fiber->stackTop - argCount;
        for (int i = 0; i < argCount; i++)
        {
            instance->values.push(args[i]);
        }
        for (int i = argCount; i < def->argCount; i++)
        {
            instance->values.push(makeNil());
        }
        fiber->stackTop -= (argCount + 1);
        PUSH(value);

        DISPATCH();
    }

    // ========================================
    // PATH 5: CLASS
    // ========================================
    else if (callee.isClass())
    {
        int classId = callee.asClassId();
        ClassDef *klass = classes[classId];

        Value value = makeClassInstance();
        ClassInstance *instance = value.asClassInstance();
        instance->klass = klass;
        instance->fields.reserve(klass->fieldCount);

        // Inicializa fields com valores default ou nil
        for (int i = 0; i < klass->fieldCount; i++)
        {
            if (i < (int)klass->fieldDefaults.size() && !klass->fieldDefaults[i].isNil())
            {
                instance->fields.push(klass->fieldDefaults[i]);
            }
            else
            {
                instance->fields.push(makeNil());
            }
        }

        // Verifica se há NativeClass na cadeia de herança (direta ou indireta)
        NativeClassDef *nativeKlass = instance->getNativeSuperclass();
        if (nativeKlass)
        {
            // Chama constructor nativo se existir (retorna userData)
            if (nativeKlass->constructor)
            {
                // O constructor nativo cria o userData
                instance->nativeUserData = nativeKlass->constructor(this, 0, nullptr);
            }
            else
            {
                // Sem constructor, aloca buffer genérico
                instance->nativeUserData = arena.Allocate(128);
                std::memset(instance->nativeUserData, 0, 128);
            }
        }

        // Substitui class por instance na stack
        fiber->stackTop[-argCount - 1] = value;

        if (klass->constructor)
        {
            if (argCount != klass->constructor->arity)
            {
                runtimeError("init() expects %d arguments, got %d",
                             klass->constructor->arity, argCount);
                return {ProcessResult::PROCESS_DONE, 0};
            }

            ENTER_CALL_FRAME_DISPATCH(klass->constructor, nullptr, argCount, "Stack overflow");
        }
        else
        {
            // Sem constructor - só remove args
            fiber->stackTop -= argCount;

            //  Não criou frame!
            DISPATCH();
        }
    }

    // ========================================
    // PATH 6: NATIVE CLASS
    // ========================================
    else if (callee.isNativeClass())
    {
        int classId = callee.asClassNativeId();
        NativeClassDef *klass = nativeClasses[classId];

        if (klass->argCount != -1 && argCount != klass->argCount)
        {
            runtimeError("Native class expects %d args, got %d",
                         klass->argCount, argCount);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        Value *args = fiber->stackTop - argCount;
        void *userData = klass->constructor(this, argCount, args);

        if (!userData)
        {
            runtimeError("Failed to create native '%s' instance",
                         klass->name->chars());
            return {ProcessResult::PROCESS_DONE, 0};
        }

        Value literal = makeNativeClassInstance(klass->persistent);
        NativeClassInstance *instance = literal.as.sClassInstance;

        instance->klass = klass;
        instance->userData = userData;

        // Remove args + callee, push instance
        fiber->stackTop -= (argCount + 1);
        PUSH(literal);

        //  Não criou frame!
        DISPATCH();
    }

    // ========================================
    // PATH 7: NATIVE STRUCT
    // ========================================
    else if (callee.isNativeStruct())
    {
        int structId = callee.asNativeStructId();
        NativeStructDef *def = nativeStructs[structId];

        void *data = arena.Allocate(def->structSize);
        std::memset(data, 0, def->structSize);

        if (def->constructor)
        {
            Value *args = fiber->stackTop - argCount;
            def->constructor(this, data, argCount, args);
        }

        Value literal = makeNativeStructInstance(def->persistent);
        NativeStructInstance *instance = literal.as.sNativeStruct;

        instance->def = def;
        instance->data = data;

        // Remove args + callee, push instance
        fiber->stackTop -= (argCount + 1);
        PUSH(literal);

        //  Não criou frame!
        DISPATCH();
    }

    // ========================================
    // PATH 8: MODULE REF
    // ========================================
    else if (callee.isModuleRef())
    {
        uint16 moduleId = (callee.as.unsignedInteger >> 16) & 0xFFFF;
        uint16 funcId = callee.as.unsignedInteger & 0xFFFF;

        if (moduleId >= modules.size())
        {
            runtimeError("Invalid module ID: %d", moduleId);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        ModuleDef *mod = modules[moduleId];
        if (funcId >= mod->functions.size())
        {
            runtimeError("Invalid function ID %d in module '%s'",
                         funcId, mod->name);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        NativeFunctionDef &func = mod->functions[funcId];

        if (func.arity != -1 && func.arity != argCount)
        {
            String *funcName;
            mod->getFunctionName(funcId, &funcName);
            runtimeError("Module '%s' expects %d args on function '%s' got %d",
                         mod->name->chars(), func.arity,
                         funcName->chars(), argCount);
            return {ProcessResult::PROCESS_DONE, 0};
        }
        SAFE_CALL_NATIVE(fiber, argCount, func.ptr(this, argCount, _args));
        //  Não criou frame!
        DISPATCH();
    }

    // ========================================
    // PATH: CLOSURE
    // ========================================
    else if (callee.isClosure())
    {
        Closure *closure = callee.asClosure();
        Function *targetFunc = functions[closure->functionId];

        if (!targetFunc)
        {
            runtimeError("Invalid closure");
            return {ProcessResult::PROCESS_DONE, 0};
        }

        if (argCount != targetFunc->arity)
        {
            runtimeError("Closure expected %d arguments but got %d",
                         targetFunc->arity, argCount);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        ENTER_CALL_FRAME_DISPATCH(targetFunc, closure, argCount, "Stack overflow");
    }

    // ========================================
    // ERRO: Tipo desconhecido
    // ========================================
    else
    {
        runtimeError("Can only call functions");
        printf("> ");
        printValue(callee);
        printf("\n");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    LOAD_FRAME();
}

op_return:
{
    Value result = POP();

    if (hasFatalError_)
    {
        STORE_FRAME();
        return {ProcessResult::ERROR, 0};
    }
    // Fecha upvalues desta frame
    if (fiber->frameCount > 0)
    {
        CallFrame *returningFrame = &fiber->frames[fiber->frameCount - 1];
        Value *frameStart = returningFrame->slots;
        while (openUpvalues != nullptr && openUpvalues->location >= frameStart)
        {
            Upvalue *upvalue = openUpvalues;
            upvalue->closed = *upvalue->location;
            upvalue->location = &upvalue->closed;
            openUpvalues = upvalue->nextOpen;
        }
    }

    bool hasFinally = false;
    if (fiber->tryDepth > 0)
    {
        for (int depth = fiber->tryDepth - 1; depth >= 0; depth--)
        {
            TryHandler &handler = fiber->tryHandlers[depth];

            if (handler.finallyIP != nullptr && !handler.inFinally)
            {
                // Marca para executar finally
                handler.pendingReturns[0] = result;
                handler.pendingReturnCount = 1;
                handler.hasPendingReturn = true;
                handler.inFinally = true;
                fiber->tryDepth = depth + 1; // Ajusta depth
                fiber->stackTop = handler.stackRestore;
                ip = handler.finallyIP;
                hasFinally = true;
                break;
            }
        }
    }

    if (hasFinally)
    {
        DISPATCH();
    }

    fiber->frameCount--;

    // Boundary for C++->script calls: stop exactly when the requested frame
    // returns, without continuing execution of the caller frame.
    if (stopOnCallReturn_ &&
        fiber == static_cast<ProcessExec *>(callReturnProcess_) &&
        fiber->frameCount == callReturnTargetFrameCount_)
    {
        CallFrame *finished = &fiber->frames[fiber->frameCount];
        fiber->stackTop = finished->slots;
        *fiber->stackTop++ = result;
        return {ProcessResult::CALL_RETURN, 0};
    }

    if (fiber->frameCount == 0)
    {
        fiber->stackTop = fiber->stack;
        *fiber->stackTop++ = result;

        fiber->state = ProcessState::DEAD;

        if (fiber == process)
        {
            process->state = ProcessState::DEAD;
        }

        return {ProcessResult::PROCESS_DONE, 0};
    }
    CallFrame *finished = &fiber->frames[fiber->frameCount];
    fiber->stackTop = finished->slots;
    *fiber->stackTop++ = result;

    LOAD_FRAME();
    DISPATCH();
    // printf("end");
}

    // ========== PROCESS/FIBER CONTROL ==========

op_array_push:
{
    uint8_t argCount = READ_BYTE();
    if (argCount != 1)
    {
        runtimeError("push() expects 1 argument");
        return {ProcessResult::ERROR, 0};
    }

    Value item = PEEK();
    Value receiver = NPEEK(argCount);
    if (!receiver.isArray())
    {
        runtimeError("push() fast opcode expects array receiver");
        return {ProcessResult::ERROR, 0};
    }

    receiver.asArray()->values.push(item);
    fiber->stackTop -= (argCount + 1);
    PUSH(receiver);
    DISPATCH();
}

op_legacy_fiber_opcode:
{
    runtimeError("Legacy fiber opcode is disabled in single-fiber mode");
    STORE_FRAME();
    return {ProcessResult::ERROR, 0};
}

op_frame:
{
    Value value = POP();
    int percent = value.isInt() ? value.asInt() : (int)value.asDouble();

    STORE_FRAME();
    return {ProcessResult::PROCESS_FRAME, percent};
}

op_exit:
{
    Value exitCode = POP();

    // Define exit code (int ou 0)
    process->exitCode = exitCode.isInt() ? exitCode.asInt() : 0;

    // Mata o processo
    process->state = ProcessState::DEAD;

    // Mata o contexto de execução do processo
    ProcessExec *f = process;
    f->state = ProcessState::DEAD;
    f->frameCount = 0;
    f->ip = nullptr;
    f->stackTop = f->stack;

    //  deixa o exitCode no topo da fiber atual para debug
    fiber->stackTop = fiber->stack;
    *fiber->stackTop++ = exitCode;

    STORE_FRAME();
    return {ProcessResult::PROCESS_DONE, 0};
}

    // ========== DEBUG ==========

op_print:
{
    uint8_t argCount = READ_BYTE();
    Value *args = fiber->stackTop - argCount;
    for (uint8_t i = 0; i < argCount; i++)
    {
        printValue(args[i]);
        // if (i < argCount - 1)
        // {
        //     printf(" "); // Espaço entre argumentos
        // }
    }
    printf("\n");

    // Remove argumentos da stack
    fiber->stackTop -= argCount;

    DISPATCH();
}

op_len:
{
    Value value = PEEK();
    if (value.isString())
    {
        DROP();
        PUSH(makeInt(value.asString()->length()));
    }
    else if (value.isArray())
    {
        DROP();
        PUSH(makeInt(value.asArray()->values.size()));
    }
    else if (value.isMap())
    {
        DROP();
        PUSH(makeInt(value.asMap()->table.count));
    }
    else
    {
        runtimeError("len() expects (string , array , map)");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    DISPATCH();
}

    // ========== PROPERTY ACCESS ==========

op_get_property:
{
    Value object = PEEK();
    Value nameValue = READ_CONSTANT();

    // printf("\nGet Object: '");
    // printValue(object);
    // printf("'\nName : '");
    // printValue(nameValue);
    // printf("'\n");

    if (!nameValue.isString())
    {
        runtimeError("Property name must be string");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    const char *name = nameValue.asStringChars();
    String *nameString = nameValue.asString();

    // === STRING METHODS ===

    if (object.isString())
    {

        if (nameString == staticNames[(int)StaticNames::LENGTH])
        {

            DROP();
            PUSH(makeInt(object.asString()->length()));
        }
        else
        {

            runtimeError("String has no property '%s'", name);
            return {ProcessResult::PROCESS_DONE, 0};
        }
    }
    else

        // === PROCESS PRIVATES (external access) ===
        if (object.isProcessInstance())
        {
            Process *proc = object.asProcess();
            if (!proc || proc->state == ProcessState::DEAD)
            {
                if (debugMode_)
                    safetimeError("GET property '%s' on dead process (returning nil)", name);
                DROP();
                PUSH(makeNil());
                DISPATCH();
            }
            int privateIdx = getProcessPrivateIndex(name);
            if (privateIdx != -1)
            {
                DROP();
                PUSH(proc->privates[privateIdx]);
            }
            else
            {
                runtimeError("Process does not support '%s' property access", name);
                return {ProcessResult::ERROR, 0};
            }

            DISPATCH();
        }
        else

            if (object.isStructInstance())
        {

            StructInstance *inst = object.asStructInstance();
            if (!inst)
            {
                runtimeError("Struct is null");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            uint8 value = 0;
            if (inst->def->names.get(nameValue.asString(), &value))
            {

                DROP();
                PUSH(inst->values[value]);
            }
            else
            {
                runtimeError("Struct '%s' has no field '%s'", inst->def->name->chars(), name);
                PUSH(makeNil());
                return {ProcessResult::PROCESS_DONE, 0};
            }
            DISPATCH();
        }
        else

            if (object.isClassInstance())
        {
            ClassInstance *instance = object.asClassInstance();

            // bool inherited = instance->klass->inherited;

            uint8_t fieldIdx;
            if (instance->klass->fieldNames.get(nameValue.asString(), &fieldIdx))
            {
                DROP();
                PUSH(instance->fields[fieldIdx]);
                DISPATCH();
            }

            // Verifica propriedades herdadas da NativeClass
            NativeProperty nativeProp;
            if (instance->getNativeProperty(nameValue.asString(), &nativeProp))
            {
                DROP(); // Remove object
                // Chama getter nativo com userData do híbrido
                Value result = nativeProp.getter(this, instance->nativeUserData);
                PUSH(result);
                DISPATCH();
            }

            runtimeError("Undefined property '%s'", name);
            PUSH(makeNil());
            return {ProcessResult::PROCESS_DONE, 0};
        }
        else

            if (object.isNativeClassInstance())
        {

            NativeClassInstance *instance = object.asNativeClassInstance();
            NativeClassDef *klass = instance->klass;
            NativeProperty prop;
            if (instance->klass->properties.get(nameValue.asString(), &prop))
            {
                DROP(); // Remove object

                //  Chama getter
                Value result = prop.getter(this, instance->userData);
                PUSH(result);
                DISPATCH();
            }

            runtimeError("Undefined property '%s' on native class '%s", nameValue.asStringChars(), klass->name->chars());
            DROP();
            PUSH(makeNil());
            return {ProcessResult::PROCESS_DONE, 0};
        }
        else

            if (object.isNativeStructInstance())
        {

            NativeStructInstance *inst = object.asNativeStructInstance();

            NativeStructDef *def = inst->def;

            NativeFieldDef field;

            if (!def->fields.get(nameValue.asString(), &field))
            {
                runtimeError("Undefined field '%s' on native struct '%s", nameValue.asStringChars(), def->name->chars());
                DROP();
                PUSH(makeNil());
                return {ProcessResult::PROCESS_DONE, 0};
            }
            char *base = (char *)inst->data;
            char *ptr = base + field.offset;

            Value result;
            switch (field.type)
            {
            case FieldType::BYTE:
            {
                result = makeByte(*(uint8 *)ptr);
                break;
            }
            case FieldType::INT:
                result = makeInt(*(int *)ptr);
                break;

            case FieldType::UINT:
                result = makeUInt(*(uint32 *)ptr);
                break;

            case FieldType::FLOAT:
                result = makeFloat(*(float *)ptr);
                break;
            case FieldType::DOUBLE:
                result = makeDouble(*(double *)ptr);
                break;

            case FieldType::BOOL:
                result = makeBool(*(bool *)ptr);
                break;

            case FieldType::POINTER:
                result = makePointer(*(void **)ptr);
                break;

            case FieldType::STRING:
            {
                String *str = *(String **)ptr;
                result = str ? makeString(str) : makeNil();
                break;
            }
            }

            DROP(); // Remove object
            PUSH(result);
            DISPATCH();
        }
        else if (object.isMap())
        {
            MapInstance *map = object.asMap();
            String *key = nameValue.asString();
            Value result;
            if (map->table.get(key, &result))
            {
                DROP();
                PUSH(result);
                DISPATCH();
            }
            else
            {
                THROW_RUNTIME_ERROR("Key '%s' not found in map", key->chars());
                DROP();
                PUSH(makeNil());
                return {ProcessResult::PROCESS_DONE, 0};
            }
        }

    runtimeError("Type does not support 'get' property access");

    printf("[Object: '");
    printValue(object);
    printf("' Property : '");
    printValue(nameValue);

    printf("']\n");

    PUSH(makeNil());
    return {ProcessResult::PROCESS_DONE, 0};
}
op_set_property:
{
    // Stack: [object, value]
    Value value = PEEK();
    Value object = PEEK2();
    Value nameValue = READ_CONSTANT();

    // printf("Set Value: '");
    // printValue(value);
    // printf("'\nObject: '");
    // printValue(object);
    // printf("'\nName : '");
    // printValue(nameValue);
    // printf("'\n");

    if (!nameValue.isString())
    {
        runtimeError("Property name must be string");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    String *propName = nameValue.asString();
    const char *name = propName->chars();

    // === STRINGS (read-only) ===
    if (object.isString())
    {
        runtimeError("Cannot set property on string (immutable)");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    // === PROCESS PRIVATES (external write) ===
    if (object.isProcessInstance())
    {
        Process *proc = object.asProcess();

        if (!proc || proc->state == ProcessState::DEAD)
        {
            if (debugMode_)
                safetimeError("SET property '%s' on dead process (ignored)", name);
            DROP();      // Remove value
            DROP();      // Remove process
            PUSH(value); // Assignment still returns value
            DISPATCH();
        }

        // Lookup private pelo nome
        int privateIdx = getProcessPrivateIndex(name);
        if (privateIdx != -1)
        {
            if ((privateIdx == (int)PrivateIndex::ID) || (privateIdx == (int)PrivateIndex::FATHER))
            {
                runtimeError("Property '%s' is readonly", name);
                return {ProcessResult::PROCESS_DONE, 0};
            }
            proc->privates[privateIdx] = value;
            DROP();      // Remove value
            DROP();      // Remove process
            PUSH(value); // Assignment retorna valor
            DISPATCH();
        }

        runtimeError("Process has no property '%s'", name);
        return {ProcessResult::PROCESS_DONE, 0};
    }

    if (object.isStructInstance())
    {

        StructInstance *inst = object.asStructInstance();
        if (!inst)
        {
            runtimeError("Struct is null ");
            return {ProcessResult::PROCESS_DONE, 0};
        }

        uint8 valueIndex = 0;
        if (inst->def->names.get(nameValue.asString(), &valueIndex))
        {
            inst->values[valueIndex] = value;
        }
        else
        {
            runtimeError("Struct '%s' has no field '%s'", inst->def->name->chars(), name);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        // Stack: [obj, value] -> queremos [value]
        DROP();      // Remove value
        DROP();      // Remove object
        PUSH(value); // Push value back - assignment returns the assigned value

        DISPATCH();
    }

    if (object.isClassInstance())
    {
        ClassInstance *instance = object.asClassInstance();

        uint8_t fieldIdx;
        if (instance->klass->fieldNames.get(nameValue.asString(), &fieldIdx))
        {
            instance->fields[fieldIdx] = value;
            // Stack: [obj, value] -> queremos [value]
            DROP();      // Remove value
            DROP();      // Remove object
            PUSH(value); // Push value back
            DISPATCH();
        }

        // Verifica propriedades herdadas da NativeClass
        NativeProperty nativeProp;
        if (instance->getNativeProperty(nameValue.asString(), &nativeProp))
        {
            if (!nativeProp.setter)
            {
                runtimeError("Property '%s' is read-only", name);
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            // Chama setter nativo
            nativeProp.setter(this, instance->nativeUserData, value);
            DROP();      // Remove value
            DROP();      // Remove object
            PUSH(value); // Push value back
            DISPATCH();
        }

        runtimeError("Undefined property '%s'", name);
        DROP(); // object
        return {ProcessResult::PROCESS_DONE, 0};
    }

    if (object.isNativeClassInstance())
    {

        NativeClassInstance *instance = object.asNativeClassInstance();
        NativeClassDef *klass = instance->klass;
        NativeProperty prop;
        if (instance->klass->properties.get(nameValue.asString(), &prop))
        {
            if (!prop.setter)
            {
                runtimeError("Property '%s' from class '%s' is read-only", nameValue.asStringChars(), klass->name->chars());
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }

            // Chama setter
            prop.setter(this, instance->userData, value);
            // Stack: [obj, value] -> queremos [value]
            DROP(); // Remove value
            DROP(); // Remove object
            PUSH(value);
            DISPATCH();
        }
    }

    if (object.isNativeStructInstance())
    {
        NativeStructInstance *inst = object.asNativeStructInstance();
        NativeStructDef *def = inst->def;

        NativeFieldDef field;
        if (!def->fields.get(nameValue.asString(), &field))
        {
            runtimeError("Undefined field '%s' in struct '%s", nameValue.asStringChars(), def->name->chars());
            DROP();
            return {ProcessResult::PROCESS_DONE, 0};
        }

        // Read-only check
        if (field.readOnly)
        {
            runtimeError("Field '%s' is read-only in struct '%s", nameValue.asStringChars(), def->name->chars());
            DROP();
            return {ProcessResult::PROCESS_DONE, 0};
        }
        char *base = (char *)inst->data;
        char *ptr = base + field.offset;
        switch (field.type)
        {
        case FieldType::BYTE:
        {
            if (!value.isByte())
            {
                runtimeError("Field expects byte");
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            *(uint8 *)ptr = (uint8)value.asByte();
            break;
        }

        case FieldType::INT:
            if (!value.isInt())
            {
                runtimeError("Field expects int");
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            *(int *)ptr = value.asInt();
            break;
        case FieldType::UINT:
            if (!value.isUInt())
            {
                runtimeError("Field expects uint");
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            *(uint32 *)ptr = value.asUInt();
            break;
        case FieldType::FLOAT:
        {
            if (!value.isNumber())
            {
                runtimeError("Field expects float");
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            *(float *)ptr = (float)value.asNumber();
            break;
        }
        case FieldType::DOUBLE:
            if (!value.isDouble())
            {
                runtimeError("Field expects double");
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            *(double *)ptr = value.asDouble();
            break;

        case FieldType::BOOL:
            if (!value.isBool())
            {
                runtimeError("Field expects bool");
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            *(bool *)ptr = value.asBool();
            break;

        case FieldType::POINTER:
            if (!value.isPointer())
            {
                runtimeError("Field expects pointer");
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            *(void **)ptr = value.asPointer();
            break;

        case FieldType::STRING:
        {
            if (!value.isString())
            {
                runtimeError("Field expects string");
                DROP();
                return {ProcessResult::PROCESS_DONE, 0};
            }
            String **fieldPtr = (String **)ptr;
            *fieldPtr = value.asString();
            break;
        }
        }

        // Stack: [obj, value] -> queremos [value]
        DROP(); // Remove value
        DROP(); // Remove object
        PUSH(value);
        DISPATCH();
    }

    runtimeError("Cannot 'set' property on this type");
    printf("[Object: '");
    printValue(object);
    printf("' Property : '");
    printValue(nameValue);

    printf("']\n");

    return {ProcessResult::PROCESS_DONE, 0};

    DISPATCH();
}
op_invoke:
{
    Value nameValue = READ_CONSTANT();
    uint8_t argCount = READ_BYTE();

    if (!nameValue.isString())
    {
        runtimeError("Method name must be string");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    const char *name = nameValue.asStringChars();
    String *nameString = nameValue.asString();
    Value receiver = NPEEK(argCount);

#define ARGS_CLEANUP() fiber->stackTop -= (argCount + 1)

    // === STRING METHODS ===
    if (receiver.isString())
    {
        String *str = receiver.asString();

        if (nameString == staticNames[(int)StaticNames::LENGTH])
        {
            int len = str->length();
            ARGS_CLEANUP();
            PUSH(makeInt(len));
        }
        else if (nameString == staticNames[(int)StaticNames::UPPER])
        {
            ARGS_CLEANUP();
            PUSH(makeString(stringPool.upper(str)));
        }
        else if (nameString == staticNames[(int)StaticNames::LOWER])
        {
            ARGS_CLEANUP();
            PUSH(makeString(stringPool.lower(str)));
        }
        else if (nameString == staticNames[(int)StaticNames::CONCAT])
        {
            if (argCount != 1)
            {
                runtimeError("concat() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value arg = PEEK();
            if (!arg.isString())
            {
                runtimeError("concat() expects string argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            String *result = stringPool.concat(str, arg.asString());
            ARGS_CLEANUP();
            PUSH(makeString(result));
        }
        else if (nameString == staticNames[(int)StaticNames::SUB])
        {
            if (argCount != 2)
            {
                runtimeError("sub() expects 2 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value start = PEEK2();
            Value end = PEEK();

            if (!start.isNumber() || !end.isNumber())
            {
                runtimeError("sub() expects 2 number arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            String *result = stringPool.substring(
                str,
                (uint32_t)start.asNumber(),
                (uint32_t)end.asNumber());
            ARGS_CLEANUP();
            PUSH(makeString(result));
        }
        else if (nameString == staticNames[(int)StaticNames::REPLACE])
        {
            if (argCount != 2)
            {
                runtimeError("replace() expects 2 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value oldStr = PEEK2();
            Value newStr = PEEK();

            if (!oldStr.isString() || !newStr.isString())
            {
                runtimeError("replace() expects 2 string arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            String *result = stringPool.replace(
                str,
                oldStr.asStringChars(),
                newStr.asStringChars());
            ARGS_CLEANUP();
            PUSH(makeString(result));
        }
        else if (nameString == staticNames[(int)StaticNames::AT])
        {
            if (argCount != 1)
            {
                runtimeError("at() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value index = PEEK();
            if (!index.isNumber())
            {
                runtimeError("at() expects number argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            String *result = stringPool.at(str, (int)index.asNumber());
            ARGS_CLEANUP();
            PUSH(makeString(result));
        }

        else if (nameString == staticNames[(int)StaticNames::CONTAINS])
        {
            if (argCount != 1)
            {
                runtimeError("contains() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value substr = PEEK();
            if (!substr.isString())
            {
                runtimeError("contains() expects string argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            bool result = stringPool.contains(str, substr.asString());
            ARGS_CLEANUP();
            PUSH(makeBool(result));
        }

        else if (nameString == staticNames[(int)StaticNames::TRIM])
        {
            String *result = stringPool.trim(str);
            ARGS_CLEANUP();
            PUSH(makeString(result));
        }

        else if (nameString == staticNames[(int)StaticNames::STARTWITH])
        {
            if (argCount != 1)
            {
                runtimeError("startsWith() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value prefix = PEEK();
            if (!prefix.isString())
            {
                runtimeError("startsWith() expects string argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            bool result = stringPool.startsWith(str, prefix.asString());
            ARGS_CLEANUP();
            PUSH(makeBool(result));
        }

        else if (nameString == staticNames[(int)StaticNames::ENDWITH])
        {
            if (argCount != 1)
            {
                runtimeError("endsWith() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value suffix = PEEK();
            if (!suffix.isString())
            {
                runtimeError("endsWith() expects string argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            bool result = stringPool.endsWith(str, suffix.asString());
            ARGS_CLEANUP();
            PUSH(makeBool(result));
        }

        else if (nameString == staticNames[(int)StaticNames::INDEXOF])
        {
            if (argCount < 1 || argCount > 2)
            {
                runtimeError("indexOf() expects 1 or 2 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value substr;
            int startIndex = 0;

            if (argCount == 1)
            {
                // index_of(substr)
                substr = PEEK();
            }
            else
            {
                // index_of(substr, startIndex)
                Value startVal = PEEK();
                substr = PEEK2();

                if (!startVal.isNumber())
                {
                    runtimeError("indexOf() startIndex must be number");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                startIndex = (int)startVal.asNumber();
            }

            if (!substr.isString())
            {
                runtimeError("indexOf() expects string argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            int result = stringPool.indexOf(
                str,
                substr.asString(),
                startIndex);
            ARGS_CLEANUP();
            PUSH(makeInt(result));
        }
        else if (nameString == staticNames[(int)StaticNames::REPEAT])
        {
            if (argCount != 1)
            {
                runtimeError("repeat() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value count = PEEK();
            if (!count.isNumber())
            {
                runtimeError("repeat() expects number argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            String *result = stringPool.repeat(str, (int)count.asNumber());
            ARGS_CLEANUP();
            PUSH(makeString(result));
        }
        else if (nameString == staticNames[(int)StaticNames::SPLIT])
        {
            if (argCount != 1)
            {
                runtimeError("split() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            Value delim = PEEK();
            if (!delim.isString())
            {
                runtimeError("split() expects string argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value result = makeArray();
            ArrayInstance *ptr = result.asArray();

            const char *strChars = str->chars();
            int strLen = str->length();
            const char *separator = delim.asString()->chars();
            int sepLen = delim.asString()->length();

            // CASO 1: Separador Vazio ou Nulo ("")
            // Comportamento: Divide caractere a caractere ["a", "b", "c"]
            if (!separator || sepLen == 0)
            {
                ptr->values.reserve(strLen);
                for (int i = 0; i < strLen; i++)
                {
                    // Cria string de 1 char
                    char buf[2] = {strChars[i], '\0'};

                    ptr->values.push(makeString(createString(buf, 1)));
                }
            }
            else
            {
                // CASO 2: Split Normal

                const char *start = strChars;
                const char *end = strChars + strLen;
                const char *current = start;
                const char *found = nullptr;

                while ((found = strstr(current, separator)) != nullptr)
                {
                    int partLen = found - current;

                    ptr->values.push(makeString(createString(current, partLen)));

                    // Avança ponteiro
                    current = found + sepLen;
                }

                // Adiciona o que sobrou da string (após o último separador)
                // Ex: "a,b,c" -> após o último "b," sobra "c"
                int remaining = end - current;
                if (remaining >= 0)
                {
                    ptr->values.push(makeString(createString(current, remaining)));
                }
            }

            ARGS_CLEANUP();
            PUSH(result);
        }

        else
        {
            runtimeError("String has no method '%s'", name);
            return {ProcessResult::PROCESS_DONE, 0};
        }
        DISPATCH();
    }

    // === ARRAY METHODS ===
    if (receiver.isArray())
    {
        ArrayInstance *arr = receiver.asArray();
        uint32 size = arr->values.size();
        if (nameString == staticNames[(int)StaticNames::PUSH])
        {
            if (argCount != 1)
            {
                runtimeError("push() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            Value item = PEEK();
            arr->values.push(item);

            ARGS_CLEANUP();

            PUSH(receiver);
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::POP])
        {
            if (argCount != 0)
            {
                runtimeError("pop() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            if (size == 0)
            {
                Warning("Cannot pop from empty array");
                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }
            else
            {
                Value result = arr->values.back();
                arr->values.pop();
                ARGS_CLEANUP();
                PUSH(result);
            }
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::BACK])
        {
            if (argCount != 0)
            {
                runtimeError("back() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            if (size == 0)
            {
                Warning("Cannot get back from empty array");
                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }
            else
            {
                Value result = arr->values.back();
                ARGS_CLEANUP();
                PUSH(result);
            }
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::LENGTH])
        {
            if (argCount != 0)
            {
                runtimeError("lenght() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            ARGS_CLEANUP();
            PUSH(makeInt(size));
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::CLEAR])
        {
            if (argCount != 0)
            {
                runtimeError("clear() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            arr->values.clear();
            ARGS_CLEANUP();
            PUSH(receiver);
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::REMOVE])
        {
            if (argCount != 1)
            {
                runtimeError("remove() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value index = PEEK();
            if (!index.isNumber())
            {
                runtimeError("remove() expects number argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            arr->values.remove((int)index.asNumber());
            ARGS_CLEANUP();
            PUSH(receiver);
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::INSERT])
        {
            if (argCount != 2)
            {
                runtimeError("insert() expects 2 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            Value index = NPEEK(1);
            if (!index.isNumber())
            {
                runtimeError("insert() expects number argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            int valueindex = (int)index.asNumber();
            if (valueindex < 0 || valueindex > arr->values.size())
            {
                runtimeError("insert() index out of range");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            Value item = NPEEK(0);
            arr->values.insert(valueindex, item);
            ARGS_CLEANUP();
            PUSH(receiver);
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::FIND])
        {
            if (argCount != 1)
            {
                runtimeError("find() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            Value value = NPEEK(0);
            int foundIndex = -1;

            for (uint32 i = 0; i < size; i++)
            {
                if (valuesEqual(arr->values[i], value))
                {
                    foundIndex = i;
                    break;
                }
            }
            ARGS_CLEANUP();
            PUSH(makeInt(foundIndex));
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::CONTAINS])
        {
            if (argCount != 1)
            {
                runtimeError("contains() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            Value value = NPEEK(0);
            bool found = false;
            for (uint32 i = 0; i < size; i++)
            {
                if (valuesEqual(arr->values[i], value))
                {
                    found = true;
                    break;
                }
            }
            ARGS_CLEANUP();
            PUSH(makeBool(found));
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::REVERSE])
        {
            if (argCount != 0)
            {
                runtimeError("reverse() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            arr->values.reverse();

            ARGS_CLEANUP();
            PUSH(receiver);
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::SLICE])
        {
            if (argCount < 1 || argCount > 2)
            {
                runtimeError("slice() expects (start, size)");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value startVal = NPEEK(argCount - 1);
            Value endVal = NPEEK(argCount - 2);

            if (!startVal.isNumber() || !endVal.isNumber())
            {
                runtimeError("slice() expects numbers arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            int start = (int)startVal.asNumber();
            int end = (int)endVal.asNumber();

            if (start < 0)
                start = size + start;
            if (end < 0)
                end = size + end;

            if (start < 0)
                start = 0;
            if (end > size)
                end = size;
            if (start > end)
                start = end;

            Value newArray = makeArray();
            ArrayInstance *newArr = newArray.asArray();
            for (int i = start; i < end; i++)
            {
                newArr->values.push(arr->values[i]);
            }

            ARGS_CLEANUP();
            PUSH(newArray);
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::CONCAT])
        {
            if (argCount != 1)
            {
                runtimeError("concat() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            Value value = NPEEK(0);

            if (!value.isArray())
            {
                runtimeError("concat() expects array argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            ArrayInstance *other = value.asArray();
            Value newArray = makeArray();
            ArrayInstance *newArr = newArray.asArray();
            for (uint32 i = 0; i < size; i++)
            {
                newArr->values.push(arr->values[i]);
            }
            for (uint32 i = 0; i < other->values.size(); i++)
            {
                newArr->values.push(other->values[i]);
            }

            ARGS_CLEANUP();
            PUSH(newArray);
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::FIRST])
        {
            if (argCount != 0)
            {
                runtimeError("first() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            if (size == 0)
            {
                ARGS_CLEANUP();
                PUSH(makeNil());
            }
            else
            {
                ARGS_CLEANUP();
                PUSH(arr->values[0]);
            }
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::LAST])
        {
            if (argCount != 0)
            {
                runtimeError("last() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            if (size == 0)
            {
                ARGS_CLEANUP();
                PUSH(makeNil());
            }
            else
            {
                ARGS_CLEANUP();
                PUSH(arr->values.back());
            }
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::FILL])
        {
            if (argCount != 1)
            {
                runtimeError("fill() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value fillValue = PEEK();

            for (uint32 i = 0; i < size; i++)
            {
                arr->values[i] = fillValue;
            }

            ARGS_CLEANUP();
            PUSH(receiver);
            DISPATCH();
        }

        else
        {
            runtimeError("Array has no method '%s'", name);
            return {ProcessResult::PROCESS_DONE, 0};
        }
    }

    // === MAP METHODS ===
    if (receiver.isMap())
    {
        MapInstance *map = receiver.asMap();

        if (nameString == staticNames[(int)StaticNames::HAS])
        {
            if (argCount != 1)
            {
                runtimeError("has() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value key = PEEK();

            if (!key.isString())
            {
                runtimeError("Map key must be string");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            bool exists = map->table.exist(key.asString());
            ARGS_CLEANUP();
            PUSH(makeBool(exists));
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::REMOVE])
        {
            if (argCount != 1)
            {
                runtimeError("remove() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value key = PEEK();

            if (!key.isString())
            {
                runtimeError("Map key must be string");
                ARGS_CLEANUP();
                PUSH(makeNil());
                DISPATCH();
            }

            map->table.set(key.asString(), makeNil());
            ARGS_CLEANUP();
            PUSH(makeNil());
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::CLEAR])
        {
            if (argCount != 0)
            {
                runtimeError("clear() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            map->table.destroy();
            ARGS_CLEANUP();
            PUSH(makeNil());
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::LENGTH])
        {
            if (argCount != 0)
            {
                runtimeError("lenght() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            ARGS_CLEANUP();
            PUSH(makeInt(map->table.count));
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::KEYS])
        {
            if (argCount != 0)
            {
                runtimeError("keys() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            // Retorna array de keys
            Value keys = makeArray();
            ArrayInstance *keysInstance = keys.asArray();

            map->table.forEach([&](String *key, Value value)
                               { keysInstance->values.push(makeString(key)); });

            ARGS_CLEANUP();
            PUSH(keys);
            DISPATCH();
        }
        else if (nameString == staticNames[(int)StaticNames::VALUES])
        {
            if (argCount != 0)
            {
                runtimeError("values() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            // Retorna array de values
            Value values = makeArray();
            ArrayInstance *valueInstance = values.asArray();

            map->table.forEach([&](String *key, Value value)
                               { valueInstance->values.push(value); });

            ARGS_CLEANUP();
            PUSH(values);
            DISPATCH();
        }
    }

    // === CLASS INSTANCE METHODS ===
    if (receiver.isClassInstance())
    {
        ClassInstance *instance = receiver.asClassInstance();
  
        Function *method;
        if (instance->getMethod(nameValue.asString(), &method))
        {
            if (argCount != method->arity)
            {
                runtimeError("Method '%s' expects %d arguments, got %d", name, method->arity, argCount);
                return {ProcessResult::PROCESS_DONE, 0};
            }

            //  Debug::dumpFunction(method);
            fiber->stackTop[-argCount - 1] = receiver;

            ENTER_CALL_FRAME_DISPATCH_STORE(method, nullptr, argCount, "Stack overflow in method!");
        }

        // Verifica métodos herdados da NativeClass
        NativeMethod nativeMethod;
        if (instance->getNativeMethod(nameValue.asString(), &nativeMethod))
        {
            // 1. Calcular OFFSET (seguro contra realloc)
            size_t _slot = (fiber->stackTop - fiber->stack) - argCount - 1;

            // 2. Calcular args via offset
            Value *_args = &fiber->stack[_slot + 1];

            // 3. Chamada
            int _rets = nativeMethod(this, instance->nativeUserData, argCount, _args);

            // 4. RECALCULAR destino (seguro contra realloc)
            Value *_dest = &fiber->stack[_slot];

            // 5. Processar retornos
            if (_rets > 0)
            {
                Value *_src = fiber->stackTop - _rets;
                if (_src != _dest)
                {
                    std::memmove(_dest, _src, _rets * sizeof(Value));
                }
                fiber->stackTop = _dest + _rets;
            }
            else
            {
                *_dest = makeNil();
                fiber->stackTop = _dest + 1;
            }
            DISPATCH();
        }

        runtimeError("Instance '%s' has no method '%s'", instance->klass->name->chars(), name);
        return {ProcessResult::PROCESS_DONE, 0};
    }

    if (receiver.isNativeClassInstance())
    {
        // printValueNl(receiver);
        // printValueNl(nameValue);

        NativeClassInstance *instance = receiver.asNativeClassInstance();
        NativeClassDef *klass = instance->klass;

        NativeMethod method;
        if (!instance->klass->methods.get(nameValue.asString(), &method))
        {
            runtimeError("Native class '%s' has no method '%s'", klass->name->chars(), name);
            return {ProcessResult::PROCESS_DONE, 0};
        }

        size_t calleeSlot = (fiber->stackTop - fiber->stack) - argCount - 1;
        Value *argsPtr = &fiber->stack[calleeSlot + 1];
        int numReturns = method(this, instance->userData, argCount, argsPtr);
        Value *dest = &fiber->stack[calleeSlot];
        if (numReturns > 0)
        {
            Value *src = fiber->stackTop - numReturns;
            if (src != dest)
            {
                std::memmove(dest, src, numReturns * sizeof(Value));
            }
            fiber->stackTop = dest + numReturns;
        }
        else
        {
            *dest = makeNil();
            fiber->stackTop = dest + 1;
        }

        DISPATCH();
    }

    // === BUFFER METHODS ===
    if (receiver.isBuffer())
    {
        BufferInstance *buf = receiver.asBuffer();
        size_t totalSize = buf->count * buf->elementSize;

        // buf.fill(value)
        if (nameString == staticNames[(int)StaticNames::FILL])
        {
            if (argCount != 1)
            {
                runtimeError("fill() expects 1 argument");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value fillValue = PEEK();

            switch (buf->type)
            {
            case BufferType::UINT8:
            {

                uint8_t val = fillValue.asByte();
                memset(buf->data, val, buf->count);
                break;
            }

            case BufferType::INT16:
            case BufferType::UINT16:
            {

                uint16_t val = (buf->type == BufferType::INT16)
                                   ? (uint16_t)fillValue.asInt()
                                   : (uint16_t)fillValue.asUInt();

                uint16_t *ptr = (uint16_t *)buf->data;

                // Técnica 1: Se valor é 0x0000 ou 0xFFFF, usa memset
                if (val == 0x0000)
                {
                    memset(buf->data, 0, buf->count * 2);
                }
                else if (val == 0xFFFF)
                {
                    memset(buf->data, 0xFF, buf->count * 2);
                }
                else
                {

                    ptr[0] = val;

                    size_t filled = 1;
                    while (filled < (size_t)buf->count)
                    {
                        size_t toCopy = (filled * 2 <= (size_t)buf->count)
                                            ? filled
                                            : buf->count - filled;
                        memcpy(ptr + filled, ptr, toCopy * sizeof(uint16_t));
                        filled += toCopy;
                    }
                }
                break;
            }

            case BufferType::INT32:
            case BufferType::UINT32:
            {

                uint32_t val = (buf->type == BufferType::INT32)
                                   ? (uint32_t)fillValue.asInt()
                                   : fillValue.asUInt();

                uint32_t *ptr = (uint32_t *)buf->data;

                if (val == 0)
                {
                    memset(buf->data, 0, buf->count * 4);
                }
                else if (val == 0xFFFFFFFF)
                {
                    memset(buf->data, 0xFF, buf->count * 4);
                }
                else
                {

                    ptr[0] = val;
                    size_t filled = 1;
                    while (filled < (size_t)buf->count)
                    {
                        size_t toCopy = (filled * 2 <= (size_t)buf->count)
                                            ? filled
                                            : buf->count - filled;
                        memcpy(ptr + filled, ptr, toCopy * sizeof(uint32_t));
                        filled += toCopy;
                    }
                }
                break;
            }

            case BufferType::FLOAT:
            {

                float val = fillValue.asFloat();
                float *ptr = (float *)buf->data;

                // Se for 0.0, usa memset
                if (val == 0.0f)
                {
                    memset(buf->data, 0, buf->count * sizeof(float));
                }
                else
                {

                    ptr[0] = val;
                    size_t filled = 1;
                    while (filled < (size_t)buf->count)
                    {
                        size_t toCopy = (filled * 2 <= (size_t)buf->count)
                                            ? filled
                                            : buf->count - filled;
                        memcpy(ptr + filled, ptr, toCopy * sizeof(float));
                        filled += toCopy;
                    }
                }
                break;
            }

            case BufferType::DOUBLE:
            {

                double val = fillValue.asDouble();
                double *ptr = (double *)buf->data;

                if (val == 0.0)
                {
                    memset(buf->data, 0, buf->count * sizeof(double));
                }
                else
                {

                    ptr[0] = val;
                    size_t filled = 1;
                    while (filled < (size_t)buf->count)
                    {
                        size_t toCopy = (filled * 2 <= (size_t)buf->count)
                                            ? filled
                                            : buf->count - filled;
                        memcpy(ptr + filled, ptr, toCopy * sizeof(double));
                        filled += toCopy;
                    }
                }
                break;
            }
            }

            ARGS_CLEANUP();
            PUSH(receiver);
            DISPATCH();
        }

        //   copy(dstOffset, srcBuffer, srcOffset, count)
        else if (nameString == staticNames[(int)StaticNames::COPY])
        {
            if (argCount != 4)
            {
                runtimeError("copy() expects 4 arguments (dstOffset, srcBuffer, srcOffset, count)");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value dstOffsetVal = NPEEK(3);
            Value srcBufferVal = NPEEK(2);
            Value srcOffsetVal = NPEEK(1);
            Value countVal = NPEEK(0);

            // Validações de tipo
            if (!dstOffsetVal.isInt())
            {
                runtimeError("copy() first argument (dstOffset) must be int");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            if (!srcBufferVal.isBuffer())
            {
                runtimeError("copy() second argument must be a buffer");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            if (!srcOffsetVal.isInt() || !countVal.isInt())
            {
                runtimeError("copy() srcOffset and count must be int");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            BufferInstance *srcBuf = srcBufferVal.asBuffer();
            int dstOffset = dstOffsetVal.asInt();
            int srcOffset = srcOffsetVal.asInt();
            int count = countVal.asInt();

            // Validações de compatibilidade
            if (buf->elementSize != srcBuf->elementSize)
            {
                runtimeError("Buffers must have compatible element sizes (dst:%zu, src:%zu)",
                             buf->elementSize, srcBuf->elementSize);
                return {ProcessResult::PROCESS_DONE, 0};
            }

            // Validações de range
            if (dstOffset < 0 || srcOffset < 0 || count < 0)
            {
                runtimeError("Offsets and count must be non-negative");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            if (srcOffset + count > srcBuf->count)
            {
                runtimeError("Source range [%d:%d] out of bounds (buffer size: %d)",
                             srcOffset, srcOffset + count, srcBuf->count);
                return {ProcessResult::PROCESS_DONE, 0};
            }

            if (dstOffset + count > buf->count)
            {
                runtimeError("Destination range [%d:%d] out of bounds (buffer size: %d)",
                             dstOffset, dstOffset + count, buf->count);
                return {ProcessResult::PROCESS_DONE, 0};
            }

            // Executa a cópia
            size_t copySize = count * buf->elementSize;
            uint8 *srcPtr = srcBuf->data + (srcOffset * srcBuf->elementSize);
            uint8 *dstPtr = buf->data + (dstOffset * buf->elementSize);
            memmove(dstPtr, srcPtr, copySize);

            ARGS_CLEANUP();
            PUSH(receiver);
            DISPATCH();
        }

        // buf.slice(start, end)
        else if (nameString == staticNames[(int)StaticNames::SLICE])
        {
            if (argCount != 2)
            {
                runtimeError("slice() expects 2 arguments (start, end)");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value startVal = PEEK2();
            Value endVal = PEEK();

            if (!startVal.isInt() || !endVal.isInt())
            {
                runtimeError("slice() expects int arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            int start = startVal.asInt();
            int end = endVal.asInt();

            //  Suporte a índices negativos (estilo Python)
            if (start < 0)
                start = buf->count + start;
            if (end < 0)
                end = buf->count + end;

            // Clamp
            if (start < 0)
                start = 0;
            if (start > buf->count)
                start = buf->count;
            if (end < 0)
                end = 0;
            if (end > buf->count)
                end = buf->count;

            if (start >= end)
            {
                runtimeError("Invalid slice range: start must be < end");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            int newCount = end - start;

            // Cria novo buffer
            Value newBufVal = makeBuffer(newCount, (int)buf->type);
            BufferInstance *newBuf = newBufVal.asBuffer();

            // Copia os dados
            size_t copySize = newCount * buf->elementSize;
            memcpy(newBuf->data, buf->data + (start * buf->elementSize), copySize);

            ARGS_CLEANUP();
            PUSH(newBufVal);
            DISPATCH();
        }

        // buf.clear()
        else if (nameString == staticNames[(int)StaticNames::CLEAR])
        {
            if (argCount != 0)
            {
                runtimeError("clear() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            memset(buf->data, 0, buf->count * buf->elementSize);
            buf->cursor = 0;

            ARGS_CLEANUP();
            PUSH(receiver);
            DISPATCH();
        }

        // buf.length()
        else if (nameString == staticNames[(int)StaticNames::LENGTH])
        {
            if (argCount != 0)
            {
                runtimeError("length() expects 0 arguments");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            ARGS_CLEANUP();
            PUSH(makeInt(buf->count));
            DISPATCH();
        } // buf.save(filename) - Salva dados RAW
        else if (nameString == staticNames[(int)StaticNames::SAVE])
        {
            if (argCount != 1)
            {
                runtimeError("save() expects 1 argument (filename)");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            Value filenameVal = PEEK();
            if (!filenameVal.isString())
            {
                runtimeError("save() expects string filename");
                return {ProcessResult::PROCESS_DONE, 0};
            }

            const char *filename = filenameVal.asStringChars();

            // Escreve dados RAW
            size_t dataSize = buf->count * buf->elementSize;
            int written = OsFileWrite(filename, buf->data, dataSize);

            if (written < 0 || (size_t)written != dataSize)
            {
                runtimeError("Failed to save buffer to '%s'", filename);
                return {ProcessResult::PROCESS_DONE, 0};
            }

            ARGS_CLEANUP();
            PUSH(receiver); // Retorna o próprio buffer (para chaining)
            DISPATCH();
        }
        else
            // ========================================
            // WRITE METHODS (avançam cursor)
            // ========================================

            // buf.writeByte(value)
            if (nameString == staticNames[(int)StaticNames::WRITE_BYTE])
            {
                if (argCount != 1)
                {
                    runtimeError("writeByte() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 1 > (int)totalSize)
                {
                    runtimeError("writeByte() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                buf->data[buf->cursor] = PEEK().asByte();
                buf->cursor += 1;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.writeShort(value) - int16
            else if (nameString == staticNames[(int)StaticNames::WRITE_SHORT])
            {
                if (argCount != 1)
                {
                    runtimeError("writeShort() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 2 > (int)totalSize)
                {
                    runtimeError("writeShort() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                int16_t value = (int16_t)PEEK().asInt();
                memcpy(buf->data + buf->cursor, &value, 2);
                buf->cursor += 2;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.writeUShort(value) - uint16
            else if (nameString == staticNames[(int)StaticNames::WRITE_USHORT])
            {
                if (argCount != 1)
                {
                    runtimeError("writeUShort() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 2 > (int)totalSize)
                {
                    runtimeError("writeUShort() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                uint16_t value = (uint16_t)PEEK().asInt();
                memcpy(buf->data + buf->cursor, &value, 2);
                buf->cursor += 2;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.writeInt(value) - int32
            else if (nameString == staticNames[(int)StaticNames::WRITE_INT])
            {
                if (argCount != 1)
                {
                    runtimeError("writeInt() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 4 > (int)totalSize)
                {
                    runtimeError("writeInt() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                int32_t value = PEEK().asInt();
                memcpy(buf->data + buf->cursor, &value, 4);
                buf->cursor += 4;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.writeUInt(value) - uint32 (aceita double para valores > 2^31)
            else if (nameString == staticNames[(int)StaticNames::WRITE_UINT])
            {
                if (argCount != 1)
                {
                    runtimeError("writeUInt() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 4 > (int)totalSize)
                {
                    runtimeError("writeUInt() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                Value val = PEEK();
                uint32_t value = val.isInt() ? (uint32_t)val.asInt() : (uint32_t)val.asDouble();
                memcpy(buf->data + buf->cursor, &value, 4);
                buf->cursor += 4;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.writeFloat(value)
            else if (nameString == staticNames[(int)StaticNames::WRITE_FLOAT])
            {
                if (argCount != 1)
                {
                    runtimeError("writeFloat() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 4 > (int)totalSize)
                {
                    runtimeError("writeFloat() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                float value = PEEK().asFloat();
                memcpy(buf->data + buf->cursor, &value, 4);
                buf->cursor += 4;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.writeDouble(value)
            else if (nameString == staticNames[(int)StaticNames::WRITE_DOUBLE])
            {
                if (argCount != 1)
                {
                    runtimeError("writeDouble() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 8 > (int)totalSize)
                {
                    runtimeError("writeDouble() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                double value = PEEK().asDouble();
                memcpy(buf->data + buf->cursor, &value, 8);
                buf->cursor += 8;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.writeString(str) - Escreve bytes da string (UTF-8)
            else if (nameString == staticNames[(int)StaticNames::WRITE_STRING])
            {
                if (argCount != 1)
                {
                    runtimeError("writeString() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                Value strVal = PEEK();
                if (!strVal.isString())
                {
                    runtimeError("writeString() expects string");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                String *str = strVal.asString();
                int length = str->length();

                if (buf->cursor < 0 || buf->cursor + length > (int)totalSize)
                {
                    runtimeError("writeString() not enough space (need %d bytes)", length);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                memcpy(buf->data + buf->cursor, str->chars(), length);
                buf->cursor += length;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // ========================================
            // READ METHODS (avançam cursor)
            // ========================================

            // buf.readByte()
            else if (nameString == staticNames[(int)StaticNames::READ_BYTE])
            {
                if (argCount != 0)
                {
                    runtimeError("readByte() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 1 > (int)totalSize)
                {
                    runtimeError("readByte() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                uint8_t value = buf->data[buf->cursor];
                buf->cursor += 1;

                ARGS_CLEANUP();
                PUSH(makeByte(value));
                DISPATCH();
            }

            // buf.readShort()
            else if (nameString == staticNames[(int)StaticNames::READ_SHORT])
            {
                if (argCount != 0)
                {
                    runtimeError("readShort() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 2 > (int)totalSize)
                {
                    runtimeError("readShort() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                int16_t value;
                memcpy(&value, buf->data + buf->cursor, 2);
                buf->cursor += 2;

                ARGS_CLEANUP();
                PUSH(makeInt(value));
                DISPATCH();
            }

            // buf.readUShort()
            else if (nameString == staticNames[(int)StaticNames::READ_USHORT])
            {
                if (argCount != 0)
                {
                    runtimeError("readUShort() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 2 > (int)totalSize)
                {
                    runtimeError("readUShort() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                uint16_t value;
                memcpy(&value, buf->data + buf->cursor, 2);
                buf->cursor += 2;

                ARGS_CLEANUP();
                PUSH(makeInt(value));
                DISPATCH();
            }

            // buf.readInt()
            else if (nameString == staticNames[(int)StaticNames::READ_INT])
            {
                if (argCount != 0)
                {
                    runtimeError("readInt() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 4 > (int)totalSize)
                {
                    runtimeError("readInt() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                int32_t value;
                memcpy(&value, buf->data + buf->cursor, 4);
                buf->cursor += 4;

                ARGS_CLEANUP();
                PUSH(makeInt(value));
                DISPATCH();
            }

            // buf.readUInt() - Retorna como double (para valores > 2^31)
            else if (nameString == staticNames[(int)StaticNames::READ_UINT])
            {
                if (argCount != 0)
                {
                    runtimeError("readUInt() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 4 > (int)totalSize)
                {
                    runtimeError("readUInt() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                uint32_t value;
                memcpy(&value, buf->data + buf->cursor, 4);
                buf->cursor += 4;

                ARGS_CLEANUP();
                PUSH(makeDouble((double)value));
                DISPATCH();
            }

            // buf.readFloat()
            else if (nameString == staticNames[(int)StaticNames::READ_FLOAT])
            {
                if (argCount != 0)
                {
                    runtimeError("readFloat() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 4 > (int)totalSize)
                {
                    runtimeError("readFloat() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                float value;
                memcpy(&value, buf->data + buf->cursor, 4);
                buf->cursor += 4;

                ARGS_CLEANUP();
                PUSH(makeFloat(value));
                DISPATCH();
            }

            // buf.readDouble()
            else if (nameString == staticNames[(int)StaticNames::READ_DOUBLE])
            {
                if (argCount != 0)
                {
                    runtimeError("readDouble() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + 8 > (int)totalSize)
                {
                    runtimeError("readDouble() cursor %d out of bounds", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                double value;
                memcpy(&value, buf->data + buf->cursor, 8);
                buf->cursor += 8;

                ARGS_CLEANUP();
                PUSH(makeDouble(value));
                DISPATCH();
            }

            // buf.readString(length)
            else if (nameString == staticNames[(int)StaticNames::READ_STRING])
            {
                if (argCount != 1)
                {
                    runtimeError("readString() expects 1 argument (length)");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                Value lengthVal = PEEK();
                if (!lengthVal.isInt())
                {
                    runtimeError("readString() length must be int");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                int length = lengthVal.asInt();

                if (length < 0)
                {
                    runtimeError("readString() length cannot be negative");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                if (buf->cursor < 0 || buf->cursor + length > (int)totalSize)
                {
                    runtimeError("readString() not enough data (need %d bytes)", length);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                String *str = createString((const char *)(buf->data + buf->cursor),
                                           (uint32)length);

                buf->cursor += length;

                ARGS_CLEANUP();
                PUSH(makeString(str));
                DISPATCH();
            }

            // ========================================
            // CURSOR CONTROL
            // ========================================

            // buf.seek(position)
            else if (nameString == staticNames[(int)StaticNames::SEEK])
            {
                if (argCount != 1)
                {
                    runtimeError("seek() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                Value posVal = PEEK();
                if (!posVal.isInt())
                {
                    runtimeError("seek() position must be int");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                int position = posVal.asInt();

                if (position < 0 || position > (int)totalSize)
                {
                    runtimeError("seek() position %d out of bounds (size=%zu)", position, totalSize);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                buf->cursor = position;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.tell()
            else if (nameString == staticNames[(int)StaticNames::TELL])
            {
                if (argCount != 0)
                {
                    runtimeError("tell() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                ARGS_CLEANUP();
                PUSH(makeInt(buf->cursor));
                DISPATCH();
            }

            // buf.rewind()
            else if (nameString == staticNames[(int)StaticNames::REWIND])
            {
                if (argCount != 0)
                {
                    runtimeError("rewind() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                buf->cursor = 0;

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.skip(bytes)
            else if (nameString == staticNames[(int)StaticNames::SKIP])
            {
                if (argCount != 1)
                {
                    runtimeError("skip() expects 1 argument");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                Value bytesVal = PEEK();
                if (!bytesVal.isInt())
                {
                    runtimeError("skip() bytes must be int");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                int bytes = bytesVal.asInt();
                buf->cursor += bytes;

                if (buf->cursor < 0 || buf->cursor > (int)totalSize)
                {
                    runtimeError("skip() moved cursor out of bounds (%d)", buf->cursor);
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                ARGS_CLEANUP();
                PUSH(receiver);
                DISPATCH();
            }

            // buf.remaining()
            else if (nameString == staticNames[(int)StaticNames::REMAINING])
            {
                if (argCount != 0)
                {
                    runtimeError("remaining() expects 0 arguments");
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                int remaining = totalSize - buf->cursor;

                ARGS_CLEANUP();
                PUSH(makeInt(remaining));
                DISPATCH();
            }
            else
            {
                runtimeError("Buffer has no method '%s'", name);
                return {ProcessResult::PROCESS_DONE, 0};
            }
    }

    STORE_FRAME();
    runtimeError("Cannot call method '%s' on %s", name, getValueTypeName(receiver));
    return {ProcessResult::PROCESS_DONE, 0};
}

op_super_invoke:
{
    uint8_t ownerClassId = READ_BYTE();
    uint16_t nameIdx = READ_SHORT();
    uint8_t argCount = READ_BYTE();

    Value nameValue = func->chunk->constants[nameIdx];
    String *methodName = nameValue.asString();
    Value self = NPEEK(argCount);

    if (!self.isClassInstance())
    {
        runtimeError("'super' requires an instance");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    ClassInstance *instance = self.asClassInstance();
    ClassDef *ownerClass = classes[ownerClassId];

    // printf("[RUNTIME] super.%s: ownerClassId=%d (%s), super=%s\n",
    //        methodName->chars(), ownerClassId,
    //        ownerClass->name->chars(),
    //        ownerClass->superclass ? ownerClass->superclass->name->chars() : "NULL");

    if (!ownerClass->superclass)
    {
        runtimeError("Class has no superclass");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    Function *method;

    if (compareString(methodName, staticNames[(int)StaticNames::INIT]))
    {
        method = ownerClass->superclass->constructor; // ← USA ownerClass!
        if (!method)
        {
            runtimeError("Superclass has no init()");
            return {ProcessResult::PROCESS_DONE, 0};
        }
    }
    else
    {
        // Métodos normais - procura na cadeia de herança
        method = nullptr;
        ClassDef *searchClass = ownerClass->superclass;
        while (searchClass)
        {
            if (searchClass->methods.get(methodName, &method))
            {
                break;
            }
            searchClass = searchClass->superclass;
        }

        if (!method)
        {
            runtimeError("Undefined method '%s'", methodName->chars());
            return {ProcessResult::PROCESS_DONE, 0};
        }
    }

    if (argCount != method->arity)
    {
        runtimeError("Method expects %d arguments, got %d",
                     method->arity, argCount);
        return {ProcessResult::PROCESS_DONE, 0};
    }

    ENTER_CALL_FRAME_DISPATCH_STORE(method, nullptr, argCount, "Stack overflow");
}

op_gosub:
{
    int16 off = (int16)READ_SHORT(); // lê u16 mas cast para signed
    if (fiber->gosubTop >= GOSUB_MAX)
        runtimeError("gosub stack overflow");
    fiber->gosubStack[fiber->gosubTop++] = ip; // retorno
    ip += off;                                 // forward/back
    DISPATCH();
}

op_return_sub:
{
    if (fiber->gosubTop > 0)
    {
        ip = fiber->gosubStack[--fiber->gosubTop];
        DISPATCH();
    }
    return {ProcessResult::PROCESS_DONE, 0};
}

op_define_array:
{
    uint16_t count = READ_SHORT();
    Value array = makeArray();
    ArrayInstance *instance = array.asArray();
    instance->values.resize(count);
    for (int i = count - 1; i >= 0; i--)
    {
        instance->values[i] = POP();
    }
    PUSH(array);
    DISPATCH();
}
op_define_map:
{
    uint16_t count = READ_SHORT();

    Value map = makeMap();
    MapInstance *inst = map.asMap();

    for (int i = 0; i < count; i++)
    {
        Value value = POP();
        Value key = POP();

        if (!key.isString())
        {
            runtimeError("Map key must be string");
            PUSH(makeNil());
            DISPATCH();
        }

        inst->table.set(key.asString(), value);
    }

    PUSH(map);
    DISPATCH();
}
op_set_index:
{
    Value value = POP();
    Value index = POP();
    Value container = POP();

    // printValue(value);
    // printf(" value \n");
    // printValue(index);
    // printf(" index \n");
    // printValue(container);
    // printf(" container \n");

    if (container.isArray())
    {
        if (!index.isNumber())
        {

            runtimeError("Array index must be an number");
            return {ProcessResult::ERROR, 0};
        }

        ArrayInstance *arr = container.asArray();
        int i = (int)index.asNumber();
        uint32 size = arr->values.size();

        // Negative index
        if (i < 0)
            i += size;

        if (i < 0 || i >= size)
        {
            runtimeError("Array index %d out of bounds (size=%d)", i, size);
            return {ProcessResult::ERROR, 0};
        }
        else
        {
            arr->values[i] = value;
        }

        PUSH(value); // Assignment returns value
        DISPATCH();
    }

    // === MAP  ===
    if (container.isMap())
    {
        if (!index.isString())
        {
            runtimeError("Map key must be string");
            return {ProcessResult::ERROR, 0};
        }

        MapInstance *map = container.asMap();
        map->table.set(index.asString(), value);

        PUSH(value); // Assignment returns value
        DISPATCH();
    }

    // == BUFER ==
    if (container.isBuffer())
    {
        BufferInstance *buffer = container.asBuffer();

        if (!index.isInt())
        {
            runtimeError("Buffer index must be integer");
            return {ProcessResult::ERROR, 0};
        }

        int idx = index.asInt();

        if (idx < 0 || idx >= buffer->count)
        {
            THROW_RUNTIME_ERROR("Buffer index %d out of bounds (size=%d)", idx, buffer->count);
            return {ProcessResult::ERROR, 0};
        }

        switch (buffer->type)
        {

        case BufferType::UINT8:
        {
            buffer->data[idx] = value.asByte();
            break;
        }
        case BufferType::INT16:
        {
            ((int16 *)buffer->data)[idx] = (int16)value.asInt();
            break;
        }
        case BufferType::UINT16:
        {
            ((uint16 *)buffer->data)[idx] = (uint16)value.asUInt();
            break;
        }
        case BufferType::INT32:
        {
            ((int32 *)buffer->data)[idx] = (int32)value.asInt();
            break;
        }
        case BufferType::UINT32:
        {
            ((uint32 *)buffer->data)[idx] = (uint32)value.asUInt();
            break;
        }
        case BufferType::FLOAT:
        {
            ((float *)buffer->data)[idx] = (float)value.asDouble();
            break;
        }
        case BufferType::DOUBLE:
        {
            ((double *)buffer->data)[idx] = value.asDouble();
            break;
        }
        default:
        {
            runtimeError("Invalid buffer type");
            return {ProcessResult::PROCESS_DONE, 0};
        }
        }
        PUSH(value);
        DISPATCH();
    }

    // STRING é imutável!
    if (container.isString())
    {
        runtimeError("Strings are immutable");
        return {ProcessResult::ERROR, 0};
    }

    runtimeError("Cannot index assign this type");
    PUSH(value);
    return {ProcessResult::PROCESS_DONE, 0};
    DISPATCH();
}
op_get_index:
{
    Value index = POP();
    Value container = POP();

    // printValue(index);
    // printf("\n");
    // printValue(container);
    // printf("\n");

    // === ARRAY ===
    if (container.isArray())
    {
        if (!index.isNumber())
        {
            runtimeError("Array index must be a number");
            return {ProcessResult::ERROR, 0};
        }

        ArrayInstance *arr = container.asArray();
        int i = (int)index.asNumber();
        uint32 size = arr->values.size();

        //  Negative index (Python-style)
        if (i < 0)
            i += size;

        if (i < 0 || i >= size)
        {
            runtimeError("Array index %d out of bounds (size=%d)", i, size);
            return {ProcessResult::ERROR, 0};
        }
        else
        {
            PUSH(arr->values[i]);
        }
        DISPATCH();
    }

    // === STRING ===
    if (container.isString())
    {
        if (!index.isInt())
        {
            runtimeError("String index must be integer");
            return {ProcessResult::ERROR, 0};
        }

        String *str = container.asString();
        String *result = stringPool.at(str, index.asInt());
        PUSH(makeString(result));
        DISPATCH();
    }

    // === MAP   ===
    if (container.isMap())
    {
        if (!index.isString())
        {
            runtimeError("Map key must be string");
            return {ProcessResult::ERROR, 0};
        }

        MapInstance *map = container.asMap();
        Value result;

        if (map->table.get(index.asString(), &result))
        {
            PUSH(result);
        }
        else
        {
            // Key não existe - retorna nil
            PUSH(makeNil());
        }
        DISPATCH();
    }

    // === BUFER ===
    if (container.isBuffer())
    {
        if (!index.isInt())
        {
            runtimeError("Buffer index must be integer");
            PUSH(makeNil());
            return {ProcessResult::PROCESS_DONE, 0};
        }

        BufferInstance *buffer = container.asBuffer();
        int idx = index.asInt();

        if (idx < 0 || idx >= buffer->count)
        {
            runtimeError("Buffer index %d out of bounds (size=%d)", idx, buffer->count);
            PUSH(makeNil());
            return {ProcessResult::PROCESS_DONE, 0};
        }

        size_t offset = idx * get_type_size(buffer->type);
        uint8 *ptr = buffer->data + offset;

        switch (buffer->type)
        {
        case BufferType::UINT8:
        {
            PUSH(makeDouble((double)(*ptr)));
            break;
        }
        case BufferType::INT16:
        {
            PUSH(makeDouble((double)(*(int16 *)ptr)));
            break;
        }
        case BufferType::UINT16:
        {
            PUSH(makeDouble((double)(*(uint16 *)ptr)));
            break;
        }
        case BufferType::INT32:
        {
            PUSH(makeDouble((double)(*(int32 *)ptr)));
            break;
        }
        case BufferType::UINT32:
        {
            PUSH(makeDouble((double)(*(uint32 *)ptr)));
            break;
        }
        case BufferType::FLOAT:
        {
            PUSH(makeDouble((double)(*(float *)ptr)));
            break;
        }

        case BufferType::DOUBLE:
        {
            PUSH(makeDouble(*(double *)ptr));
            break;
        }

        default:
        {
            runtimeError("Invalid buffer type");
            PUSH(makeNil());
            return {ProcessResult::PROCESS_DONE, 0};
        }
        }

        DISPATCH();
    }

    runtimeError("Cannot index this type");
    PUSH(makeNil());
    return {ProcessResult::PROCESS_DONE, 0};
}

op_foreach_start:
{
    Value arr = PEEK(); // [array]

    // printValueNl(arr);

    if (!arr.isArray())
    {
        runtimeError("foreach requires an array");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    PUSH(makeInt(0)); // [array, 0]
    DISPATCH();
}

op_iter_next:
{

    Value iter = POP();
    Value seq = POP();

    if (!seq.isArray())
    {
        runtimeError(" Iterator next Type is not iterable");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    ArrayInstance *array = seq.as.array;
    int index = iter.isNil() ? 0 : iter.as.integer + 1;

    if (index < (int)array->values.size())
    {
        PUSH(makeInt(index));
        PUSH(makeBool(true));
    }
    else
    {
        PUSH(makeNil());
        PUSH(makeBool(false));
    }
    DISPATCH();
}

op_iter_value:
{

    Value iter = POP();
    Value seq = POP();

    if (!seq.isArray())
    {
        runtimeError("Iterator Type is not iterable");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    ArrayInstance *array = seq.as.array;
    int index = iter.as.integer;

    if (index < 0 || index >= (int)array->values.size())
    {
        runtimeError("Iterator out of bounds");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    PUSH(array->values[index]);

    DISPATCH();
}

op_copy2:
{
    Value b = NPEEK(0);
    Value a = NPEEK(1);
    PUSH(a);
    PUSH(b);
    DISPATCH();
}

op_swap:
{
    Value a = POP();
    Value b = POP();
    PUSH(a);
    PUSH(b);
    DISPATCH();
}

op_discard:
{
    uint8_t count = READ_BYTE();
    fiber->stackTop -= count;

    DISPATCH();
}

op_try:
{
    uint16_t catchAddr = READ_SHORT();
    uint16_t finallyAddr = READ_SHORT();

    if (fiber->tryDepth >= TRY_MAX)
    {
        runtimeError("Try-catch nesting too deep");
        return {ProcessResult::PROCESS_DONE, 0};
    }

    TryHandler &handler = fiber->tryHandlers[fiber->tryDepth];
    handler.catchIP = catchAddr == 0xFFFF ? nullptr : func->chunk->code + catchAddr;
    handler.finallyIP = finallyAddr == 0xFFFF ? nullptr : func->chunk->code + finallyAddr;
    handler.stackRestore = fiber->stackTop;
    handler.inFinally = false;
    handler.pendingError = makeNil();
    handler.hasPendingError = false;
    handler.catchConsumed = false;

    fiber->tryDepth++;
    DISPATCH();
}

op_pop_try:
{
    if (fiber->tryDepth > 0)
    {
        fiber->tryDepth--;
    }
    DISPATCH();
}
op_enter_catch:
{
    if (fiber->tryDepth > 0)
    {
        fiber->tryHandlers[fiber->tryDepth - 1].hasPendingError = false;
    }
    DISPATCH();
}
op_enter_finally:
{
    if (fiber->tryDepth > 0)
    {
        fiber->tryHandlers[fiber->tryDepth - 1].inFinally = true;
    }
    DISPATCH();
}

op_throw:
{
    Value error = POP();
    bool handlerFound = false;

    while (fiber->tryDepth > 0)
    {
        TryHandler &handler = fiber->tryHandlers[fiber->tryDepth - 1];

        if (handler.inFinally)
        {
            handler.pendingError = error;
            handler.hasPendingError = true;
            fiber->tryDepth--;
            continue;
        }

        fiber->stackTop = handler.stackRestore;

        // Tem catch?
        if (handler.catchIP != nullptr && !handler.catchConsumed)
        {
            handler.catchConsumed = true;

            PUSH(error);
            ip = handler.catchIP;
            handlerFound = true;

            break;
        }

        // Só finally?
        else if (handler.finallyIP != nullptr)
        {
            handler.pendingError = error;
            handler.hasPendingError = true;
            handler.inFinally = true;
            ip = handler.finallyIP;
            handlerFound = true;
            break;
        }

        fiber->tryDepth--;
    }

    if (!handlerFound)
    {
        char buffer[256];
        valueToBuffer(error, buffer, sizeof(buffer));
        runtimeError("Uncaught exception: %s", buffer);

        return {ProcessResult::PROCESS_DONE, 0};
    }
    DISPATCH();
}

op_exit_finally:
{
    if (fiber->tryDepth > 0)
    {
        TryHandler &handler = fiber->tryHandlers[fiber->tryDepth - 1];
        handler.inFinally = false;

        if (handler.hasPendingReturn)
        {
            Value pendingReturns[TryHandler::MAX_PENDING_RETURNS];
            uint8_t returnCount = handler.pendingReturnCount;
            for (int i = 0; i < returnCount; i++)
            {
                pendingReturns[i] = handler.pendingReturns[i];
            }
            handler.hasPendingReturn = false;
            handler.pendingReturnCount = 0;
            fiber->tryDepth--;

            // Procura próximo finally
            bool hasAnotherFinally = false;
            for (int depth = fiber->tryDepth - 1; depth >= 0; depth--)
            {
                TryHandler &next = fiber->tryHandlers[depth];
                if (next.finallyIP != nullptr && !next.inFinally)
                {
                    for (int i = 0; i < returnCount; i++)
                    {
                        next.pendingReturns[i] = pendingReturns[i];
                    }
                    next.pendingReturnCount = returnCount;
                    next.hasPendingReturn = true;
                    next.inFinally = true;
                    fiber->tryDepth = depth + 1;
                    ip = next.finallyIP;
                    hasAnotherFinally = true;
                    break;
                }
            }

            if (!hasAnotherFinally)
            {
                // Executa return de verdade
                fiber->frameCount--;

                if (fiber->frameCount == 0)
                {
                    fiber->stackTop = fiber->stack;
                    for (int i = 0; i < returnCount; i++)
                    {
                        *fiber->stackTop++ = pendingReturns[i];
                    }
                    fiber->state = ProcessState::DEAD;

                    if (fiber == process)
                    {
                        process->state = ProcessState::DEAD;
                    }

                    STORE_FRAME();
                    return {ProcessResult::PROCESS_DONE, 0};
                }

                CallFrame *finished = &fiber->frames[fiber->frameCount];
                fiber->stackTop = finished->slots;
                for (int i = 0; i < returnCount; i++)
                {
                    *fiber->stackTop++ = pendingReturns[i];
                }

                LOAD_FRAME();
            }

            DISPATCH();
        }

        if (handler.hasPendingError)
        {
            Value error = handler.pendingError;
            handler.hasPendingError = false;
            fiber->tryDepth--;

            // Re-throw: procura próximo handler
            bool handlerFound = false;

            for (int depth = fiber->tryDepth - 1; depth >= 0; depth--)
            {
                TryHandler &nextHandler = fiber->tryHandlers[depth];

                if (nextHandler.inFinally)
                {
                    nextHandler.pendingError = error;
                    nextHandler.hasPendingError = true;
                    continue;
                }

                fiber->stackTop = nextHandler.stackRestore;

                if (nextHandler.catchIP != nullptr && !nextHandler.catchConsumed)
                {
                    nextHandler.catchConsumed = true;
                    PUSH(error);
                    ip = nextHandler.catchIP;
                    handlerFound = true;
                    fiber->tryDepth = depth + 1;
                    break;
                }
                else if (nextHandler.finallyIP != nullptr)
                {
                    nextHandler.pendingError = error;
                    nextHandler.hasPendingError = true;
                    nextHandler.inFinally = true;
                    ip = nextHandler.finallyIP;
                    handlerFound = true;
                    fiber->tryDepth = depth + 1;
                    break;
                }
            }

            if (!handlerFound)
            {
                char buffer[256];
                valueToBuffer(error, buffer, sizeof(buffer));
                runtimeError("Uncaught exception: %s", buffer);
                return {ProcessResult::PROCESS_DONE, 0};
            }
        }
        else
        {
            // Sem erro nem return pendente, só remove handler
            fiber->tryDepth--;
        }
    }
    DISPATCH();
}

    // ============================================
    // MATH: UNARY OPERATORS (1 Argumento)
    // ============================================

op_sin:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("sin() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeDouble(std::sin(val)));
    DISPATCH();
}
op_cos:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("cos() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeDouble(std::cos(val)));
    DISPATCH();
}

op_tan:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("tan() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeDouble(std::tan(val)));
    DISPATCH();
}

op_asin:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("asin() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeDouble(std::asin(val)));
    DISPATCH();
}

op_acos:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("acos() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeDouble(std::acos(val)));
    DISPATCH();
}

op_atan:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("atan() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeDouble(std::atan(val)));
    DISPATCH();
}

op_sqrt:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("sqrt() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    if (val < 0)
    {
        runtimeError("sqrt() of negative number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    PUSH(makeDouble(std::sqrt(val)));
    DISPATCH();
}
op_abs:
{
    Value v = POP();
    if (v.isInt())
    {
        PUSH(makeInt(std::abs(v.asInt())));
    }
    else if (v.isDouble())
    {
        PUSH(makeDouble(std::abs(v.asDouble())));
    }
    else
    {
        runtimeError("abs() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    DISPATCH();
}
op_log:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("log() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    if (val <= 0)
    {
        runtimeError("log() domain error");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    PUSH(makeDouble(std::log(val)));
    DISPATCH();
}

op_floor:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("floor() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeInt((int)std::floor(val))); // Retorna Int
    DISPATCH();
}

op_ceil:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("ceil() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeInt((int)std::ceil(val))); // Retorna Int
    DISPATCH();
}

    // --- CONVERSÃO ---
op_deg:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("deg() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeDouble(val * 57.29577951308232));
    DISPATCH();
}

op_rad:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("rad() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();
    PUSH(makeDouble(val * 0.017453292519943295));
    DISPATCH();
}

op_exp:
{
    Value v = POP();
    if (!v.isNumber())
    {
        runtimeError("exp() expects a number");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double val = v.isInt() ? (double)v.asInt() : v.asDouble();

    // std::exp calcula e^x
    PUSH(makeDouble(std::exp(val)));
    DISPATCH();
}

    // --- BINÁRIOS (2 Argumentos) ---
op_atan2:
{
    Value vx = POP(); // X (topo)
    Value vy = POP(); // Y (abaixo)
    if (!vx.isNumber() || !vy.isNumber())
    {
        runtimeError("atan2(y, x) operands must be numbers");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double x = vx.isInt() ? (double)vx.asInt() : vx.asDouble();
    double y = vy.isInt() ? (double)vy.asInt() : vy.asDouble();
    PUSH(makeDouble(std::atan2(y, x)));
    DISPATCH();
}

op_pow:
{
    Value vexp = POP();  // Expoente
    Value vbase = POP(); // Base
    if (!vexp.isNumber() || !vbase.isNumber())
    {
        runtimeError("pow(base, exp) operands must be numbers");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    double exp = vexp.isInt() ? (double)vexp.asInt() : vexp.asDouble();
    double base = vbase.isInt() ? (double)vbase.asInt() : vbase.asDouble();
    PUSH(makeDouble(std::pow(base, exp)));
    DISPATCH();
}

op_clock:
{
    PUSH(makeDouble(static_cast<double>(clock()) / CLOCKS_PER_SEC));
    DISPATCH();
}

op_new_buffer:
{
    // A pilha tem [size, type] <- Topo
    Value type = POP();
    Value size = POP();

    if (!type.isInt())
    {
        THROW_RUNTIME_ERROR("Buffer type must be an integer.");
    }
    int t = type.asInt();
    if (t < 0 || t >= ((int)BufferType::DOUBLE + 1))
    {
        THROW_RUNTIME_ERROR("Invalid buffer type: %d", (int)type.asBuffer()->type);
    }

    if (size.isNumber())
    {

        if (!size.isInt())
        {
            THROW_RUNTIME_ERROR("Buffer size must be an integer.");
        }

        int count = size.asInt();

        if (count < 0)
        {
            THROW_RUNTIME_ERROR("Buffer size cannot be negative.");
        }

        PUSH(makeBuffer(count, t));
    }
    else if (size.isString())
    {
        const char *filename = size.asStringChars();

        int fileSize = OsFileSize(filename);
        if (fileSize < 0)
        {
            THROW_RUNTIME_ERROR("Failed to get size of file '%s'", filename);
        }
        if (fileSize == 0)
        {
            THROW_RUNTIME_ERROR("File '%s' is empty.", filename);
        }

        size_t elementSize = get_type_size((BufferType)t);
        if (fileSize % elementSize != 0)
        {
            THROW_RUNTIME_ERROR("File size %d is not a multiple of element size %zu",
                                fileSize, elementSize);
        }

        int count = fileSize / elementSize;

        // 3. Cria o buffer
        Value bufferVal = makeBuffer(count, t);
        if (bufferVal.asBuffer()->data == nullptr)
        {
            THROW_RUNTIME_ERROR("Failed to allocate buffer of %d elements (type %d)", count, (int)type.asBuffer()->type);
        }
        BufferInstance *buf = bufferVal.asBuffer();
        int bytesRead = OsFileRead(filename, buf->data, fileSize);
        if (bytesRead < 0 || bytesRead != fileSize)
        {
            THROW_RUNTIME_ERROR("Failed to read data from '%s' (%d bytes read, expected %d)",
                                filename, bytesRead, fileSize);
        }

        PUSH(bufferVal);

        DISPATCH();
    }
    else
    {
        THROW_RUNTIME_ERROR("Buffer size must be an integer or a string.");
    }

    DISPATCH();
}
op_free:
{
    Value object = POP();
    bool freed = false;

    // Info("Freeing %s", valueTypeToString(object.type));

    if (object.isStructInstance())
    {

        StructInstance *instance = object.asStructInstance();
        if (!instance)
        {
            runtimeError("Struct is null");
            return {ProcessResult::PROCESS_DONE, 0};
        }
        // Info("Free  Struct address: %p", (void*)instance);
        instance->marked = 1;
        freed = true;
    }
    else if (object.isClassInstance())
    {
        ClassInstance *instance = object.asClassInstance();
        if (!instance)
        {
            runtimeError("Class instance is nil");
            return {ProcessResult::PROCESS_DONE, 0};
        }
        instance->marked = 1;
        freed = true;
    }
    else if (object.isNativeClassInstance())
    {
        NativeClassInstance *instance = object.asNativeClassInstance();
        if (!instance)
        {
            runtimeError("Native class instance is nil");
            return {ProcessResult::PROCESS_DONE, 0};
        }
        instance->marked = 1;
        freed = true;
    }
    else if (object.isNativeStructInstance())
    {
        NativeStructInstance *instance = object.asNativeStructInstance();
        if (!instance)
        {
            runtimeError("Native struct instance is nil");
            return {ProcessResult::PROCESS_DONE, 0};
        }
        // Info("Free  Native Struct address: %p", (void*)instance);
        instance->marked = 1;
        freed = true;
    }
    else if (object.isBuffer())
    {
        BufferInstance *instance = object.asBuffer();
        if (!instance)
        {
            runtimeError("Buffer instance is nil");
            return {ProcessResult::PROCESS_DONE, 0};
        }
        instance->marked = 1;
        freed = true;
    }
    else if (object.isMap())
    {

        MapInstance *instance = object.asMap();
        if (!instance)
        {
            runtimeError("Map instance is nil");
            return {ProcessResult::PROCESS_DONE, 0};
        }
        instance->marked = 1;
        freed = true;
    }
    else if (object.isArray())
    {
        ArrayInstance *instance = object.asArray();
        if (!instance)
        {
            runtimeError("Array instance is nil");
            return {ProcessResult::PROCESS_DONE, 0};
        }
        instance->marked = 1;
        freed = true;
    }

    // Warning("Object not in category to be freed: %s", valueTypeToString(object.type));
    PUSH(makeBool(freed));
    DISPATCH();
}

    // ========== CLOSURES ==========

op_closure:
{
    Value funcVal = READ_CONSTANT();
    int funcID = funcVal.asFunctionId();
    Function *function = functions[funcID];
    Value closure = makeClosure();
    Closure *closurePtr = closure.as.closure;
    closurePtr->functionId = funcID;
    closurePtr->upvalueCount = function->upvalueCount;

    closurePtr->upvalues.clear();

    for (int i = 0; i < function->upvalueCount; i++)
    {
        uint8 isLocal = READ_BYTE();
        uint8 index = READ_BYTE();

        if (isLocal)
        {
            Value *local = &stackStart[index];

            // Procura na lista openUpvalues
            Upvalue *prev = nullptr;
            Upvalue *upvalue = openUpvalues;

            while (upvalue != nullptr && upvalue->location > local)
            {
                prev = upvalue;
                upvalue = upvalue->nextOpen;
            }

            if (upvalue != nullptr && upvalue->location == local)
            {
                closurePtr->upvalues.push(upvalue);
            }
            else
            {
                Upvalue *created = createUpvalue(local);
                created->nextOpen = upvalue;

                if (prev == nullptr)
                {
                    openUpvalues = created;
                }
                else
                {
                    prev->nextOpen = created;
                }

                closurePtr->upvalues.push(created);
            }
        }
        else
        {
            if (!frame->closure)
            {
                runtimeError("Cannot capture upvalue without enclosing closure");
                return {ProcessResult::PROCESS_DONE, 0};
            }
            if (index >= frame->closure->upvalueCount)
            {
                runtimeError("Upvalue index %d out of bounds (count=%d)", index, frame->closure->upvalueCount);
                return {ProcessResult::PROCESS_DONE, 0};
            }
            closurePtr->upvalues.push(frame->closure->upvalues[index]);
        }
    }

    PUSH(closure);
    DISPATCH();
}

op_get_upvalue:
{
    uint8 slot = READ_BYTE();

    if (!frame->closure)
    {
        runtimeError("Upvalue access outside closure");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    if (slot >= frame->closure->upvalueCount)
    {
        runtimeError("Upvalue index %d out of bounds (count=%d)", slot, frame->closure->upvalueCount);
        return {ProcessResult::PROCESS_DONE, 0};
    }

    PUSH(*frame->closure->upvalues[slot]->location);
    DISPATCH();
}

op_set_upvalue:
{
    uint8 slot = READ_BYTE();

    if (!frame->closure)
    {
        runtimeError("Upvalue access outside closure");
        return {ProcessResult::PROCESS_DONE, 0};
    }
    if (slot >= frame->closure->upvalueCount)
    {
        runtimeError("Upvalue index %d out of bounds (count=%d)", slot, frame->closure->upvalueCount);
        return {ProcessResult::PROCESS_DONE, 0};
    }

    *frame->closure->upvalues[slot]->location = PEEK();
    DISPATCH();
}

op_close_upvalue:
{
    Value *last = fiber->stackTop - 1;
    while (openUpvalues != nullptr && openUpvalues->location >= last)
    {
        Upvalue *upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->nextOpen;
    }
    DROP();
    DISPATCH();
}

op_return_n:
{
    uint8_t count = READ_BYTE();

    // Save the N return values (they're on top of stack)
    Value results[256];
    for (int i = count - 1; i >= 0; i--)
    {
        results[i] = POP();
    }

    if (hasFatalError_)
    {
        STORE_FRAME();
        return {ProcessResult::ERROR, 0};
    }

    // Close upvalues for this frame
    if (fiber->frameCount > 0)
    {
        CallFrame *returningFrame = &fiber->frames[fiber->frameCount - 1];
        Value *frameStart = returningFrame->slots;
        while (openUpvalues != nullptr && openUpvalues->location >= frameStart)
        {
            Upvalue *upvalue = openUpvalues;
            upvalue->closed = *upvalue->location;
            upvalue->location = &upvalue->closed;
            openUpvalues = upvalue->nextOpen;
        }
    }

    // Handle try/finally - note: multi-return in finally may not work correctly
    bool hasFinally = false;
    if (fiber->tryDepth > 0)
    {
        for (int depth = fiber->tryDepth - 1; depth >= 0; depth--)
        {
            TryHandler &handler = fiber->tryHandlers[depth];

            if (handler.finallyIP != nullptr && !handler.inFinally)
            {
                // Save all return values for multi-return through finally
                int n = (count < TryHandler::MAX_PENDING_RETURNS) ? count : TryHandler::MAX_PENDING_RETURNS;
                for (int i = 0; i < n; i++)
                {
                    handler.pendingReturns[i] = results[i];
                }
                handler.pendingReturnCount = (uint8_t)n;
                handler.hasPendingReturn = true;
                handler.inFinally = true;
                fiber->tryDepth = depth + 1;
                fiber->stackTop = handler.stackRestore;
                ip = handler.finallyIP;
                hasFinally = true;
                break;
            }
        }
    }

    if (hasFinally)
    {
        DISPATCH();
    }

    fiber->frameCount--;

    // Boundary for C++->script calls (multi-return variant).
    if (stopOnCallReturn_ &&
        fiber == static_cast<ProcessExec *>(callReturnProcess_) &&
        fiber->frameCount == callReturnTargetFrameCount_)
    {
        CallFrame *finished = &fiber->frames[fiber->frameCount];
        fiber->stackTop = finished->slots;
        for (int i = 0; i < count; i++)
        {
            *fiber->stackTop++ = results[i];
        }
        return {ProcessResult::CALL_RETURN, 0};
    }

    if (fiber->frameCount == 0)
    {
        fiber->stackTop = fiber->stack;
        // Push all return values
        for (int i = 0; i < count; i++)
        {
            *fiber->stackTop++ = results[i];
        }

        fiber->state = ProcessState::DEAD;

        if (fiber == process)
        {
            process->state = ProcessState::DEAD;
        }

        return {ProcessResult::PROCESS_DONE, 0};
    }

    CallFrame *finished = &fiber->frames[fiber->frameCount];
    fiber->stackTop = finished->slots;

    // Push all return values onto the stack
    for (int i = 0; i < count; i++)
    {
        *fiber->stackTop++ = results[i];
    }

    LOAD_FRAME();
    DISPATCH();
}

op_type:
{
    Value nameVal = POP();
    String *name = nameVal.asString();
    ProcessDef *procDef = nullptr;
    if (!processesMap.get(name, &procDef))
    {
        runtimeError("Unknown process type: %s", name->chars());
        STORE_FRAME();
        return {ProcessResult::PROCESS_DONE, 0};
    }
    PUSH(makeInt(procDef->index));
    DISPATCH();
}

op_proc:
{
    Value idVal = POP();
    if (!idVal.isNumber())
    {
        runtimeError("proc expects a number (process id)");
        STORE_FRAME();
        return {ProcessResult::PROCESS_DONE, 0};
    }
    uint32 id = (uint32)idVal.asNumber();
    Process *target = findProcessById(id);
    if (!target)
    {
        PUSH(makeNil());
    }
    else
    {
        PUSH(makeProcessInstance(target));
    }
    DISPATCH();
}

op_get_id:
{
    Value blueprintVal = POP();
    if (!blueprintVal.isInt())
    {
        PUSH(makeInt(-1));
        DISPATCH();
    }
    int targetBlueprint = blueprintVal.asInt();
    bool found = false;
    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        Process *p = aliveProcesses[i];
        if (p && p->blueprint == targetBlueprint && p->state != ProcessState::DEAD)
        {
            PUSH(makeInt(p->id));
            found = true;
            break;
        }
    }
    if (!found) PUSH(makeInt(-1));
    DISPATCH();
}

// Cleanup macros

#undef READ_BYTE
#undef READ_SHORT
}

#endif // USE_COMPUTED_GOTO
