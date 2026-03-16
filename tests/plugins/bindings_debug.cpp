#include "interpreter.hpp"
#include "plugin.hpp"

namespace
{
int nativeDebugCurrentFunction(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;

    DebugLocation location;
    if (!vm->getCurrentLocation(&location) || !location.functionName)
    {
        vm->pushNil();
        return 1;
    }

    vm->push(vm->makeString(location.functionName));
    return 1;
}

int nativeDebugCurrentLine(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;

    DebugLocation location;
    if (!vm->getCurrentLocation(&location))
    {
        vm->pushInt(-1);
        return 1;
    }

    vm->pushInt(location.line);
    return 1;
}

int nativeDebugFrameCount(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(vm->getCallFrameCount());
    return 1;
}

int nativeDebugFrameFunction(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isNumber())
    {
        vm->runtimeError("_dbgFrameFunction(depth) expects one numeric depth");
        return 0;
    }

    DebugFrameInfo info;
    if (!vm->getCallFrameInfo(static_cast<int>(args[0].asNumber()), &info) || !info.functionName)
    {
        vm->pushNil();
        return 1;
    }

    vm->push(vm->makeString(info.functionName));
    return 1;
}

int nativeDebugFrameLine(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isNumber())
    {
        vm->runtimeError("_dbgFrameLine(depth) expects one numeric depth");
        return 0;
    }

    DebugFrameInfo info;
    if (!vm->getCallFrameInfo(static_cast<int>(args[0].asNumber()), &info))
    {
        vm->pushInt(-1);
        return 1;
    }

    vm->pushInt(info.line);
    return 1;
}

int nativeDebugFrameSlots(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isNumber())
    {
        vm->runtimeError("_dbgFrameSlots(depth) expects one numeric depth");
        return 0;
    }

    DebugFrameInfo info;
    if (!vm->getCallFrameInfo(static_cast<int>(args[0].asNumber()), &info))
    {
        vm->pushInt(-1);
        return 1;
    }

    vm->pushInt(info.slotCount);
    return 1;
}

int nativeDebugGlobal(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isString())
    {
        vm->runtimeError("_dbgGlobal(name) expects one string name");
        return 0;
    }

    Value value;
    if (!vm->tryGetGlobal(args[0].asStringChars(), &value))
    {
        vm->pushNil();
        return 1;
    }

    vm->push(value);
    return 1;
}

int nativeDebugLocal(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isNumber())
    {
        vm->runtimeError("_dbgLocal(slot) expects one numeric slot");
        return 0;
    }

    Value value;
    if (!vm->getFrameSlotValue(0, static_cast<int>(args[0].asNumber()), &value))
    {
        vm->pushNil();
        return 1;
    }

    vm->push(value);
    return 1;
}

int nativeDebugGlobalCount(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(vm->getGlobalCount());
    return 1;
}

void registerModule(Interpreter *vm)
{
    vm->addModule("bindings_debug");
    vm->registerNative("_dbgCurrentFunction", nativeDebugCurrentFunction, 0);
    vm->registerNative("_dbgCurrentLine", nativeDebugCurrentLine, 0);
    vm->registerNative("_dbgFrameCount", nativeDebugFrameCount, 0);
    vm->registerNative("_dbgFrameFunction", nativeDebugFrameFunction, 1);
    vm->registerNative("_dbgFrameLine", nativeDebugFrameLine, 1);
    vm->registerNative("_dbgFrameSlots", nativeDebugFrameSlots, 1);
    vm->registerNative("_dbgGlobal", nativeDebugGlobal, 1);
    vm->registerNative("_dbgLocal", nativeDebugLocal, 1);
    vm->registerNative("_dbgGlobalCount", nativeDebugGlobalCount, 0);
}
} // namespace

BU_DEFINE_PLUGIN_SIMPLE("bindings_debug", "1.0", "BuGL Tests", registerModule)
