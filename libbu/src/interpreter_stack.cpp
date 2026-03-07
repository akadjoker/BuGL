#include "interpreter.hpp"
#include "pool.hpp"
#include "opcode.hpp"
#include "debug.hpp"
#include <string>



void Interpreter::checkType(int index, ValueType expected, const char *funcName)
{
    Value v = peek(index);
    if (v.type != expected)
    {
        runtimeError("%s expects %s at index %d, got %s",
                     funcName,
                     valueTypeToString(expected),
                     index,
                     valueTypeToString(v.type));
    }
}



// int Interpreter::toInt(int index)
// {
//     checkType(index, ValueType::INT, "toInt");
//     return peek(index).asInt();
// }

// ===== STACK API =====

const Value &Interpreter::peek(int index)
{
    WDIV_ASSERT(currentExec() != nullptr, "No current fiber");

    int top = getTop();
    int realIndex;

    if (index < 0)
        realIndex = top + index; // -1 → top-1, -2 → top-2
    else
        realIndex = index; // 0 → 0, 1 → 1

    if (realIndex < 0 || realIndex >= top)
    {
        runtimeError("Stack index %d out of bounds (size=%d)", index, top);
        static Value null = makeNil();
        return null;
    }

    return currentExec()->stack[realIndex];
}

int Interpreter::getTop()
{
    WDIV_ASSERT(currentExec() != nullptr, "No current fiber");
    return static_cast<int>(currentExec()->stackTop - currentExec()->stack);
}

void Interpreter::setTop(int index)
{
    WDIV_ASSERT(currentExec() != nullptr, "No current fiber");
    if (index < 0 || index > STACK_MAX)
    {
        runtimeError("Invalid stack index");
        return;
    }
    currentExec()->stackTop = currentExec()->stack + index;
}

void Interpreter::push(Value value)
{

    if (currentExec()->stackTop >= currentExec()->stack + STACK_MAX)
    {
        runtimeError("Stack overflow");
        return;
    }
    *currentExec()->stackTop++ = value;
}

Value Interpreter::pop()
{

    if (currentExec()->stackTop <= currentExec()->stack)
    {
        runtimeError("Stack underflow");
        return makeNil();
    }
    return *--currentExec()->stackTop;
}

//  const Value &Interpreter::peek(int distance)
// {

//     //    int stackSize = currentExec()->stackTop - currentExec()->stack;
//     ptrdiff_t stackSize = currentExec()->stackTop - currentExec()->stack;

//     if (distance < 0 || distance >= stackSize)
//     {
//         runtimeError("Stack peek out of bounds: distance=%d, size=%d",
//                      distance, stackSize);
//         static const Value null = makeNil();
//         return null;
//     }
//     return currentExec()->stackTop[-1 - distance];
// }

// Type checking
ValueType Interpreter::getType(int index)
{
    return peek(index).type;
}

bool Interpreter::isInt(int index)
{
    return peek(index).type == ValueType::INT;
}

bool Interpreter::isDouble(int index)
{
    return peek(index).type == ValueType::DOUBLE;
}

bool Interpreter::isString(int index)
{
    return peek(index).type == ValueType::STRING;
}

bool Interpreter::isBool(int index)
{
    return peek(index).type == ValueType::BOOL;
}

bool Interpreter::isNil(int index)
{
    return peek(index).type == ValueType::NIL;
}

bool Interpreter::isFunction(int index)
{
    return peek(index).type == ValueType::FUNCTION;
}

void Interpreter::pushInt(int n)
{
    push(makeInt(n));
}

void Interpreter::pushFloat(float f)
{
    push(makeDouble(f));
}

void Interpreter::pushPointer(void *p)
{
    push(makePointer(p));
}

void Interpreter::pushByte(uint8 b)
{
    push(makeInt(b));
}

void Interpreter::pushDouble(double d)
{
    push(makeDouble(d));
}

void Interpreter::pushString(const char *s)
{
    push(makeString(s));
}

void Interpreter::pushBool(bool b)
{
    push(makeBool(b));
}

void Interpreter::pushNil()
{
    push(makeNil());
}

// Type conversions
int Interpreter::toInt(int index)
{
    Value v = peek(index);
    if (!v.isInt())
    {
        runtimeError("Expected int at index %d", index);
        return 0;
    }
    return v.asInt();
}

double Interpreter::toDouble(int index)
{
    Value v = peek(index);

    if (v.isDouble())
    {
        return v.asDouble();
    }
    else if (v.isInt())
    {
        return (double)v.asInt();
    }

    runtimeError("Expected number at index %d", index);
    return 0.0;
}

const char *Interpreter::toString(int index)
{
    const Value &v = peek(index);
    if (!v.isString())
    {
        runtimeError("Expected string at index %d", index);
        return "";
    }
    return v.asString()->chars();
}

bool Interpreter::toBool(int index)
{
    Value v = peek(index);
    return isTruthy(v);
}

void Interpreter::insert(int index)
{
    // Insere topo no index, shift elementos
    WDIV_ASSERT(currentExec() != nullptr, "No current fiber");

    int top = getTop();
    if (index < 0)
        index = top + index + 1;
    if (index < 0 || index > top)
    {
        runtimeError("Invalid insert index");
        return;
    }

    Value value = pop();

    // Shift elementos para direita
    for (int i = top - 1; i >= index; i--)
    {
        currentExec()->stack[i + 1] = currentExec()->stack[i];
    }

    currentExec()->stack[index] = value;
    currentExec()->stackTop++;
}

void Interpreter::remove(int index)
{
    // Remove elemento no index
    WDIV_ASSERT(currentExec() != nullptr, "No current fiber");

    int top = getTop();
    if (index < 0)
        index = top + index;
    if (index < 0 || index >= top)
    {
        runtimeError("Invalid remove index");
        return;
    }

    // Shift elementos para esquerda
    for (int i = index; i < top - 1; i++)
    {
        currentExec()->stack[i] = currentExec()->stack[i + 1];
    }

    currentExec()->stackTop--;
}

void Interpreter::replace(int index)
{

    WDIV_ASSERT(currentExec() != nullptr, "No current fiber");

    int top = getTop();
    if (index < 0)
        index = top + index;
    if (index < 0 || index >= top)
    {
        runtimeError("Invalid replace index");
        return;
    }

    currentExec()->stack[index] = pop();
}
void Interpreter::copy(int fromIndex, int toIndex)
{

    WDIV_ASSERT(currentExec() != nullptr, "No current fiber");

    int top = getTop();
    if (fromIndex < 0)
        fromIndex = top + fromIndex;
    if (toIndex < 0)
        toIndex = top + toIndex;

    if (fromIndex < 0 || fromIndex >= top ||
        toIndex < 0 || toIndex >= top)
    {
        runtimeError("Invalid copy indices");
        return;
    }

    currentExec()->stack[toIndex] = currentExec()->stack[fromIndex];
}

void Interpreter::rotate(int index, int n)
{
    // Roda n elementos a partir de index
    // rotate(-3, 1): ABC → CAB
    WDIV_ASSERT(currentExec() != nullptr, "No current fiber");

    int top = getTop();
    if (index < 0)
        index = top + index;

    if (index < 0 || index >= top || n == 0)
        return;

    int count = top - index;
    n = ((n % count) + count) % count;

    // Reverse [index, index+count-n)
    for (int i = 0; i < (count - n) / 2; i++)
    {
        Value temp = currentExec()->stack[index + i];
        currentExec()->stack[index + i] = currentExec()->stack[index + count - n - 1 - i];
        currentExec()->stack[index + count - n - 1 - i] = temp;
    }

    // Reverse [index+count-n, index+count)
    for (int i = 0; i < n / 2; i++)
    {
        Value temp = currentExec()->stack[index + count - n + i];
        currentExec()->stack[index + count - n + i] = currentExec()->stack[index + count - 1 - i];
        currentExec()->stack[index + count - 1 - i] = temp;
    }

    // Reverse [index, index+count)
    for (int i = 0; i < count / 2; i++)
    {
        Value temp = currentExec()->stack[index + i];
        currentExec()->stack[index + i] = currentExec()->stack[index + count - 1 - i];
        currentExec()->stack[index + count - 1 - i] = temp;
    }
}

bool Interpreter::callFunction(Function *func, int argCount)
{
    if (!func)
    {
        runtimeError("Cannot call null function");
        return false;
    }

    // Verifica arity
    if (argCount != func->arity)
    {
        runtimeError("Function '%s' expects %d arguments but got %d",
                     func->name->chars(), func->arity, argCount);
        return false;
    }

    Process *proc = currentProcess ? currentProcess : mainProcess;
    if (!proc)
    {
        runtimeError("No active process to call function");
        return false;
    }

    ProcessExec *fiber = proc;
    if (currentExec() && currentExec() != fiber)
    {
        runtimeError("Execution context mismatch while calling '%s'", func->name->chars());
        return false;
    }

    int stackSize = static_cast<int>(fiber->stackTop - fiber->stack);
    if (stackSize < (argCount + 1))
    {
        runtimeError("Function call '%s' is missing callee/arguments on stack",
                     func->name->chars());
        return false;
    }

    // Verifica overflow de frames
    if (fiber->frameCount >= FRAMES_MAX)
    {
        runtimeError("Stack overflow - too many nested calls");
        return false;
    }

    if (!func->chunk || func->chunk->count == 0)
    {
        runtimeError("Function '%s' has no bytecode!", func->name->chars());
        return false;
    }

//    Debug::disassembleChunk(*func->chunk, func->name->chars());

    CallFrame *frame = &fiber->frames[fiber->frameCount];
    frame->func = func;
    frame->closure = nullptr;
    frame->ip = func->chunk->code;
    frame->slots = fiber->stackTop - argCount - 1; // slot 0 = callee/self

    int targetFrames = fiber->frameCount;
    fiber->frameCount++;

    bool prevStop = stopOnCallReturn_;
    Process *prevProcess = callReturnProcess_;
    int prevTarget = callReturnTargetFrameCount_;

    stopOnCallReturn_ = true;
    callReturnProcess_ = proc;
    callReturnTargetFrameCount_ = targetFrames;

    while (true)
    {
        ProcessResult result = run_process(proc);
        if (result.reason == ProcessResult::ERROR)
        {
            stopOnCallReturn_ = prevStop;
            callReturnProcess_ = prevProcess;
            callReturnTargetFrameCount_ = prevTarget;
            return false;
        }
        if (result.reason == ProcessResult::CALL_RETURN)
        {
            stopOnCallReturn_ = prevStop;
            callReturnProcess_ = prevProcess;
            callReturnTargetFrameCount_ = prevTarget;
            return true;
        }
        if (result.reason == ProcessResult::PROCESS_DONE)
        {
            stopOnCallReturn_ = prevStop;
            callReturnProcess_ = prevProcess;
            callReturnTargetFrameCount_ = prevTarget;
            runtimeError("Function '%s' ended process before returning to caller",
                         func->name->chars());
            return false;
        }
    }
}

bool Interpreter::callFunction(const char *name, int argCount)
{
    // Lookup function por nome
    String *funcName = createString(name);
    Function *func = nullptr;

    if (!functionsMap.get(funcName, &func))
    {

        runtimeError("Undefined function: %s", name);
        return false;
    }


    if (getTop() < argCount)
    {
        runtimeError("Not enough arguments on stack to call '%s'", name);
        return false;
    }

    // callFunction expects stack layout: callee, arg1..argN
    push(makeFunction(func->index));
    rotate(getTop() - argCount - 1, 1); // move callee before args
    return callFunction(func, argCount);
}

Function* Interpreter::getFunction(const char *name)
{
    // Tentar nome directo primeiro
    String *funcName = createString(name);
    Function *func = nullptr;

    if (functionsMap.get(funcName, &func))
    {
        return func;
    }

    // Tentar com prefixo __main__$
    char prefixedName[256];
    snprintf(prefixedName, sizeof(prefixedName), "__main__$%s", name);

    String *prefixedString = createString(prefixedName);
    if (functionsMap.get(prefixedString, &func))
    {
        return func;
    }

    return nullptr;
}

bool Interpreter::callFunctionAuto(const char *name, int argCount)
{
    Function *func = getFunction(name);

    if (!func)
    {
        runtimeError("Undefined function: %s", name);
        return false;
    }

    if (getTop() < argCount)
    {
        runtimeError("Not enough arguments on stack to call '%s'", name);
        return false;
    }

    // callFunction expects stack layout: callee, arg1..argN
    push(makeFunction(func->index));
    rotate(getTop() - argCount - 1, 1); // move callee before args
    return callFunction(func, argCount);
}

bool Interpreter::callMethod(Value instance, const char *methodName, int argCount, Value *args)
{
    if (!instance.isClassInstance())
    {
        runtimeError("callMethod: value is not a class instance");
        return false;
    }

    ClassInstance *inst = instance.asClassInstance();
    String *name = createString(methodName);
    Function *method = nullptr;

    if (!inst->getMethod(name, &method))
    {
        // Method not found - not necessarily an error (optional methods like start/render)
        return false;
    }

    if (argCount != method->arity)
    {
        runtimeError("Method '%s' expects %d arguments, got %d",
                     methodName, method->arity, argCount);
        return false;
    }

    Process *proc = currentProcess ? currentProcess : mainProcess;
    if (!proc)
    {
        runtimeError("No active process to call method '%s'", methodName);
        return false;
    }
    ProcessExec *fiber = proc;
    if (currentExec() && currentExec() != fiber)
    {
        runtimeError("Execution context mismatch while calling method '%s'", methodName);
        return false;
    }

    if (fiber->stackTop + argCount + 1 > fiber->stack + STACK_MAX)
    {
        runtimeError("Stack overflow calling method '%s'", methodName);
        return false;
    }

    int savedFrameCount = fiber->frameCount;
    Value *savedStackTop = fiber->stackTop;

    // Push self (slot 0) + args
    *fiber->stackTop++ = instance;
    for (int i = 0; i < argCount; i++)
    {
        *fiber->stackTop++ = args[i];
    }

    if (fiber->frameCount >= FRAMES_MAX)
    {
        runtimeError("Stack overflow calling method '%s'", methodName);
        fiber->stackTop = savedStackTop;
        return false;
    }

    CallFrame *frame = &fiber->frames[fiber->frameCount++];
    frame->func = method;
    frame->closure = nullptr;
    frame->ip = method->chunk->code;
    frame->slots = fiber->stackTop - argCount - 1; // self is before args

    bool prevStop = stopOnCallReturn_;
    Process *prevProcess = callReturnProcess_;
    int prevTarget = callReturnTargetFrameCount_;

    stopOnCallReturn_ = true;
    callReturnProcess_ = proc;
    callReturnTargetFrameCount_ = savedFrameCount;

    // Execute the method
    while (true)
    {
        ProcessResult result = run_process(proc);
        if (result.reason == ProcessResult::ERROR)
        {
            stopOnCallReturn_ = prevStop;
            callReturnProcess_ = prevProcess;
            callReturnTargetFrameCount_ = prevTarget;
            fiber->stackTop = savedStackTop;
            return false;
        }
        if (result.reason == ProcessResult::CALL_RETURN)
        {
            stopOnCallReturn_ = prevStop;
            callReturnProcess_ = prevProcess;
            callReturnTargetFrameCount_ = prevTarget;
            return true;
        }
        if (result.reason == ProcessResult::PROCESS_DONE)
        {
            stopOnCallReturn_ = prevStop;
            callReturnProcess_ = prevProcess;
            callReturnTargetFrameCount_ = prevTarget;
            fiber->stackTop = savedStackTop;
            runtimeError("Method '%s' ended process before returning to caller", methodName);
            return false;
        }
    }
}

Process *Interpreter::callProcess(ProcessDef *proc, int argCount)
{
    if (!proc)
    {
        runtimeError("Cannot call null process");
        return nullptr;
    }

    Function *processFunc = proc->frames[0].func;

    // Verifica arity
    if (argCount != processFunc->arity)
    {
        runtimeError("Process '%s' expects %d arguments but got %d",
                     proc->name->chars(), processFunc->arity, argCount);
        return nullptr;
    }

    // Spawn process
    Process *instance = spawnProcess(proc);

    if (!instance)
    {
        runtimeError("Failed to spawn process");
        return nullptr;
    }

    // Se tem argumentos, inicializa
    if (argCount > 0)
    {
        ProcessExec *procFiber = instance;
        int localSlot = 0;

        for (int i = 0; i < argCount; i++)
        {
            Value arg = currentExec()->stackTop[-argCount + i];

            if (i < (int)proc->argsNames.size() && proc->argsNames[i] != 255)
            {
                // Arg mapeia para um private (x, y, etc.) - copia direto
                instance->privates[proc->argsNames[i]] = arg;
            }
            else
            {
                // Arg é um local normal
                procFiber->stack[localSlot] = arg;
                localSlot++;
            }
        }
        procFiber->stackTop = procFiber->stack + localSlot;

        // Remove args da stack atual
        currentExec()->stackTop -= argCount;
    }

    // ID e FATHER
    instance->privates[(int)PrivateIndex::ID] = makeInt(instance->id);
    if (currentProcess && currentProcess->id > 0)
    {
        instance->privates[(int)PrivateIndex::FATHER] = makeInt(currentProcess->id);
    }

    // if (hooks.onStart)
    // {
    //     hooks.onStart(instance);
    // }

    return instance;
}

Process *Interpreter::callProcess(const char *name, int argCount)
{
    String *procName = createString(name);
    ProcessDef *proc = nullptr;

    if (!processesMap.get(procName, &proc))
    {
       
        runtimeError("Undefined process: %s", name);
        return nullptr;
    }

 
    return callProcess(proc, argCount);
}
