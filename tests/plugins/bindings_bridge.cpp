#include "interpreter.hpp"
#include "plugin.hpp"

namespace
{
struct BridgeHandle
{
    int nativeValue{0};
    bool persistent{false};
};

struct BridgeStats
{
    int persistentCtorCount{0};
    int persistentDtorCount{0};
    int ephemeralCtorCount{0};
    int ephemeralDtorCount{0};
    int callbackCount{0};
    int globalCallCount{0};
    int lastEventValue{0};
};

BridgeStats g_bridgeStats;

void resetBridgeStats()
{
    g_bridgeStats = {};
}

void *bridgeCtor(Interpreter *vm, int argCount, Value *args, bool persistent)
{
    (void)vm;
    (void)argCount;
    (void)args;

    BridgeHandle *handle = new BridgeHandle();
    handle->persistent = persistent;
    if (persistent)
        g_bridgeStats.persistentCtorCount++;
    else
        g_bridgeStats.ephemeralCtorCount++;
    return handle;
}

void *bridgePersistentCtor(Interpreter *vm, int argCount, Value *args)
{
    return bridgeCtor(vm, argCount, args, true);
}

void *bridgeEphemeralCtor(Interpreter *vm, int argCount, Value *args)
{
    return bridgeCtor(vm, argCount, args, false);
}

void bridgeDtor(Interpreter *vm, void *instance)
{
    (void)vm;
    BridgeHandle *handle = static_cast<BridgeHandle *>(instance);
    if (!handle)
        return;

    if (handle->persistent)
        g_bridgeStats.persistentDtorCount++;
    else
        g_bridgeStats.ephemeralDtorCount++;

    delete handle;
}

int bridgeBumpNative(Interpreter *vm, void *instance, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isNumber())
    {
        vm->runtimeError("bumpNative() expects one numeric value");
        return 0;
    }

    BridgeHandle *handle = static_cast<BridgeHandle *>(instance);
    if (!handle)
    {
        vm->pushNil();
        return 1;
    }

    handle->nativeValue += static_cast<int>(args[0].asNumber());
    vm->pushInt(handle->nativeValue);
    return 1;
}

Value bridgeGetNativeValue(Interpreter *vm, void *instance)
{
    BridgeHandle *handle = static_cast<BridgeHandle *>(instance);
    return vm->makeInt(handle ? handle->nativeValue : 0);
}

void bridgeSetNativeValue(Interpreter *vm, void *instance, Value value)
{
    BridgeHandle *handle = static_cast<BridgeHandle *>(instance);
    if (!handle)
        return;
    if (!value.isNumber())
    {
        vm->runtimeError("nativeValue expects a numeric assignment");
        return;
    }

    handle->nativeValue = static_cast<int>(value.asNumber());
}

Value bridgeGetIsPersistent(Interpreter *vm, void *instance)
{
    BridgeHandle *handle = static_cast<BridgeHandle *>(instance);
    return vm->makeBool(handle && handle->persistent);
}

int nativeBridgeReset(Interpreter *vm, int argCount, Value *args)
{
    (void)vm;
    (void)argCount;
    (void)args;
    resetBridgeStats();
    return 0;
}

int nativeBridgePersistentCtorCount(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(g_bridgeStats.persistentCtorCount);
    return 1;
}

int nativeBridgePersistentDtorCount(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(g_bridgeStats.persistentDtorCount);
    return 1;
}

int nativeBridgeEphemeralCtorCount(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(g_bridgeStats.ephemeralCtorCount);
    return 1;
}

int nativeBridgeEphemeralDtorCount(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(g_bridgeStats.ephemeralDtorCount);
    return 1;
}

int nativeBridgeCallbackCount(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(g_bridgeStats.callbackCount);
    return 1;
}

int nativeBridgeGlobalCallCount(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(g_bridgeStats.globalCallCount);
    return 1;
}

int nativeBridgeLastEventValue(Interpreter *vm, int argCount, Value *args)
{
    (void)argCount;
    (void)args;
    vm->pushInt(g_bridgeStats.lastEventValue);
    return 1;
}

int nativeBridgeInvoke(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 2 || !args[0].isClassInstance() || !args[1].isNumber())
    {
        vm->runtimeError("_bridgeInvoke(instance, value) expects (classInstance, number)");
        return 0;
    }

    const int savedTop = vm->getTop();
    Value callArgs[1] = {vm->makeInt(static_cast<int>(args[1].asNumber()))};
    g_bridgeStats.lastEventValue = callArgs[0].asInt();
    const bool ok = vm->callMethod(args[0], "onEvent", 1, callArgs);
    if (vm->getTop() > savedTop)
        vm->setTop(savedTop);
    if (ok)
        g_bridgeStats.callbackCount++;
    vm->pushBool(ok);
    return 1;
}

int nativeBridgeCallGlobal(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 2 || !args[0].isString() || !args[1].isNumber())
    {
        vm->runtimeError("_bridgeCallGlobal(name, value) expects (string, number)");
        return 0;
    }

    const int savedTop = vm->getTop();
    vm->pushInt(static_cast<int>(args[1].asNumber()));
    const bool ok = vm->callFunctionAuto(args[0].asStringChars(), 1);
    if (vm->getTop() > savedTop)
        vm->setTop(savedTop);
    if (ok)
        g_bridgeStats.globalCallCount++;
    vm->pushBool(ok);
    return 1;
}

int nativeBridgeCreate(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 2 || !args[0].isString() || !args[1].isNumber())
    {
        vm->runtimeError("_bridgeCreate(className, seed) expects (string, number)");
        return 0;
    }

    Value ctorArgs[1] = {vm->makeInt(static_cast<int>(args[1].asNumber()))};
    Value instance = vm->createClassInstance(args[0].asStringChars(), 1, ctorArgs);
    vm->push(instance);
    return 1;
}

void registerModule(Interpreter *vm)
{
    resetBridgeStats();
    vm->addModule("bindings_bridge");

    NativeClassDef *persistentBridge = vm->registerNativeClass("TestBridgePersistent", bridgePersistentCtor, bridgeDtor, 0, true);
    vm->addNativeMethod(persistentBridge, "bumpNative", bridgeBumpNative);
    vm->addNativeProperty(persistentBridge, "nativeValue", bridgeGetNativeValue, bridgeSetNativeValue);
    vm->addNativeProperty(persistentBridge, "isPersistentNative", bridgeGetIsPersistent, nullptr);

    NativeClassDef *ephemeralBridge = vm->registerNativeClass("TestBridgeEphemeral", bridgeEphemeralCtor, bridgeDtor, 0, false);
    vm->addNativeMethod(ephemeralBridge, "bumpNative", bridgeBumpNative);
    vm->addNativeProperty(ephemeralBridge, "nativeValue", bridgeGetNativeValue, bridgeSetNativeValue);
    vm->addNativeProperty(ephemeralBridge, "isPersistentNative", bridgeGetIsPersistent, nullptr);

    vm->registerNative("_bridgeReset", nativeBridgeReset, 0);
    vm->registerNative("_bridgePersistentCtorCount", nativeBridgePersistentCtorCount, 0);
    vm->registerNative("_bridgePersistentDtorCount", nativeBridgePersistentDtorCount, 0);
    vm->registerNative("_bridgeEphemeralCtorCount", nativeBridgeEphemeralCtorCount, 0);
    vm->registerNative("_bridgeEphemeralDtorCount", nativeBridgeEphemeralDtorCount, 0);
    vm->registerNative("_bridgeCallbackCount", nativeBridgeCallbackCount, 0);
    vm->registerNative("_bridgeGlobalCallCount", nativeBridgeGlobalCallCount, 0);
    vm->registerNative("_bridgeLastEventValue", nativeBridgeLastEventValue, 0);
    vm->registerNative("_bridgeInvoke", nativeBridgeInvoke, 2);
    vm->registerNative("_bridgeCallGlobal", nativeBridgeCallGlobal, 2);
    vm->registerNative("_bridgeCreate", nativeBridgeCreate, 2);
}
} // namespace

BU_DEFINE_PLUGIN_SIMPLE("bindings_bridge", "1.0", "BuGL Tests", registerModule)
