
#include "interpreter.hpp"
#include "platform.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>

namespace
{
struct GcProbeHandle
{
  int id;
  bool persistent;
  bool payloadAlive;
  int *payload;
};

struct GcProbeStats
{
  int ctorCount{0};
  int dtorCount{0};
  int manualPayloadDestroyCount{0};
  int autoPayloadDestroyCount{0};
};

GcProbeStats g_gcProbePersistentStats;
GcProbeStats g_gcProbeEphemeralStats;

void resetGcProbeStats()
{
  g_gcProbePersistentStats = {};
  g_gcProbeEphemeralStats = {};
}

GcProbeStats &gc_probe_stats(bool persistent)
{
  return persistent ? g_gcProbePersistentStats : g_gcProbeEphemeralStats;
}

bool gc_probe_release_payload(GcProbeHandle *handle, bool manual)
{
  if (!handle || !handle->payloadAlive || handle->payload == nullptr)
    return false;

  GcProbeStats &stats = gc_probe_stats(handle->persistent);
  delete handle->payload;
  handle->payload = nullptr;
  handle->payloadAlive = false;

  if (manual)
    stats.manualPayloadDestroyCount++;
  else
    stats.autoPayloadDestroyCount++;

  return true;
}

void *gc_probe_ctor(Interpreter *vm, int argCount, Value *args, bool persistent)
{
  if (argCount != 1 || !args[0].isNumber())
  {
    vm->runtimeError("%s expects one numeric id", persistent ? "GcPersistentProbe" : "GcEphemeralProbe");
    return nullptr;
  }

  GcProbeHandle *handle = new GcProbeHandle();
  handle->id = (int)args[0].asNumber();
  handle->persistent = persistent;
  handle->payloadAlive = true;
  handle->payload = new int(handle->id);

  gc_probe_stats(persistent).ctorCount++;

  return handle;
}

void *gc_persistent_probe_ctor(Interpreter *vm, int argCount, Value *args)
{
  return gc_probe_ctor(vm, argCount, args, true);
}

void *gc_ephemeral_probe_ctor(Interpreter *vm, int argCount, Value *args)
{
  return gc_probe_ctor(vm, argCount, args, false);
}

void gc_probe_dtor(Interpreter *vm, void *instance)
{
  (void)vm;
  GcProbeHandle *handle = (GcProbeHandle *)instance;
  if (!handle)
    return;

  gc_probe_release_payload(handle, false);
  gc_probe_stats(handle->persistent).dtorCount++;

  delete handle;
}

int gc_probe_id(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  GcProbeHandle *handle = (GcProbeHandle *)instance;
  if (!handle)
  {
    vm->pushNil();
    return 1;
  }

  vm->pushInt(handle->id);
  return 1;
}

int gc_probe_is_persistent(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  GcProbeHandle *handle = (GcProbeHandle *)instance;
  vm->pushBool(handle && handle->persistent);
  return 1;
}

int gc_probe_destroy_payload(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  GcProbeHandle *handle = (GcProbeHandle *)instance;
  vm->pushBool(gc_probe_release_payload(handle, true));
  return 1;
}

int gc_probe_is_payload_alive(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  GcProbeHandle *handle = (GcProbeHandle *)instance;
  vm->pushBool(handle && handle->payloadAlive && handle->payload != nullptr);
  return 1;
}

int native_gc_probe_reset(Interpreter *vm, int argCount, Value *args)
{
  (void)vm;
  (void)argCount;
  (void)args;
  resetGcProbeStats();
  return 0;
}

int native_gc_probe_persistent_ctor_count(Interpreter *vm, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  vm->pushInt(g_gcProbePersistentStats.ctorCount);
  return 1;
}

int native_gc_probe_persistent_dtor_count(Interpreter *vm, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  vm->pushInt(g_gcProbePersistentStats.dtorCount);
  return 1;
}

int native_gc_probe_ephemeral_ctor_count(Interpreter *vm, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  vm->pushInt(g_gcProbeEphemeralStats.ctorCount);
  return 1;
}

int native_gc_probe_ephemeral_dtor_count(Interpreter *vm, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  vm->pushInt(g_gcProbeEphemeralStats.dtorCount);
  return 1;
}

int native_gc_probe_persistent_manual_payload_count(Interpreter *vm, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  vm->pushInt(g_gcProbePersistentStats.manualPayloadDestroyCount);
  return 1;
}

int native_gc_probe_persistent_auto_payload_count(Interpreter *vm, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  vm->pushInt(g_gcProbePersistentStats.autoPayloadDestroyCount);
  return 1;
}

int native_gc_probe_ephemeral_manual_payload_count(Interpreter *vm, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  vm->pushInt(g_gcProbeEphemeralStats.manualPayloadDestroyCount);
  return 1;
}

int native_gc_probe_ephemeral_auto_payload_count(Interpreter *vm, int argCount, Value *args)
{
  (void)argCount;
  (void)args;
  vm->pushInt(g_gcProbeEphemeralStats.autoPayloadDestroyCount);
  return 1;
}

int native_typeid(Interpreter *vm, int argCount, Value *args)
{
  if (argCount != 1)
  {
    vm->runtimeError("typeid() expects exactly one argument");
    return 0;
  }

  auto encode = [](int kind, int index) -> int
  {
    return ((kind & 0x7f) << 24) | (index & 0x00ffffff);
  };

  const Value &arg = args[0];
  int typeId = 0;

  switch (arg.type)
  {
  case ValueType::CLASS:
    typeId = encode(1, arg.asClassId());
    break;
  case ValueType::CLASSINSTANCE:
    if (!arg.asClassInstance() || !arg.asClassInstance()->klass)
    {
      vm->runtimeError("typeid() received invalid class instance");
      return 0;
    }
    typeId = encode(1, arg.asClassInstance()->klass->index);
    break;
  case ValueType::STRUCT:
    typeId = encode(2, arg.asStructId());
    break;
  case ValueType::STRUCTINSTANCE:
    if (!arg.asStructInstance() || !arg.asStructInstance()->def)
    {
      vm->runtimeError("typeid() received invalid struct instance");
      return 0;
    }
    typeId = encode(2, arg.asStructInstance()->def->index);
    break;
  case ValueType::NATIVECLASS:
    typeId = encode(3, arg.asClassNativeId());
    break;
  case ValueType::NATIVECLASSINSTANCE:
    if (!arg.asNativeClassInstance() || !arg.asNativeClassInstance()->klass)
    {
      vm->runtimeError("typeid() received invalid native class instance");
      return 0;
    }
    typeId = encode(3, arg.asNativeClassInstance()->klass->index);
    break;
  case ValueType::NATIVESTRUCT:
    typeId = encode(4, arg.asNativeStructId());
    break;
  case ValueType::NATIVESTRUCTINSTANCE:
    if (!arg.asNativeStructInstance() || !arg.asNativeStructInstance()->def)
    {
      vm->runtimeError("typeid() received invalid native struct instance");
      return 0;
    }
    typeId = encode(4, arg.asNativeStructInstance()->def->id);
    break;
  default:
    vm->runtimeError("typeid() expects a type or instance");
    return 0;
  }

  vm->pushInt(typeId);
  return 1;
}
} // namespace

int native_print_stack(Interpreter *vm, int argCount, Value *args)
{

  if (argCount == 1)
  {
    Info("%s", args[0].asString()->chars());
  }
  vm->printStack();
  return 0;
}

static void valueToString(const Value &v, std::string &out)
{
  char buffer[256];

  switch (v.type)
  {
  case ValueType::NIL:
    out += "nil";
    break;
  case ValueType::BOOL:
    out += v.as.boolean ? "true" : "false";
    break;
  case ValueType::BYTE:
    snprintf(buffer, 256, "%u", v.as.byte);
    out += buffer;
    break;
  case ValueType::INT:
    snprintf(buffer, 256, "%d", v.as.integer);
    out += buffer;
    break;
  case ValueType::UINT:
    snprintf(buffer, 256, "%u", v.as.unsignedInteger);
    out += buffer;
    break;
  case ValueType::FLOAT:
    snprintf(buffer, 256, "%.2f", v.as.real);
    out += buffer;
    break;
  case ValueType::DOUBLE:
    snprintf(buffer, 256, "%.2f", v.as.number);
    out += buffer;
    break;
  case ValueType::STRING:
  {
    out += v.asStringChars();
    break;
  }
  case ValueType::PROCESS:
    snprintf(buffer, 256, "<process:%u>", v.as.integer);
    out += buffer;
    break;
  case ValueType::PROCESS_INSTANCE:
  {
    Process *proc = v.asProcess();
    if (!proc)
    {
      out += "<process:null>";
      break;
    }
    if (proc->name)
      snprintf(buffer, 256, "<process:%u %s>", proc->id, proc->name->chars());
    else
      snprintf(buffer, 256, "<process:%u>", proc->id);
    out += buffer;
    break;
  }
  case ValueType::ARRAY:
    out += "[array]";
    break;
  case ValueType::MAP:
    out += "{map}";
    break;
  case ValueType::BUFFER:
    out += "[buffer]";
    break;
  default:
    out += "<unknown>";
  }
}

int native_string(Interpreter *vm, int argCount, Value *args)
{
  if (argCount != 1)
  {
    vm->runtimeError("str() expects exactly one argument");
    return 0;
  }

  std::string result;
  valueToString(args[0], result);
  vm->pushString(result.c_str());
  return 1;
}

int native_classname(Interpreter *vm, int argCount, Value *args)
{
  if (argCount != 1)
  {
    vm->runtimeError("classname() expects exactly one argument");
    return 0;
  }
  const Value &arg = args[0];
  if (arg.isClassInstance())
  {
    vm->pushString(arg.as.sClass->klass->name->chars());
    return 1;
  }
  if (arg.isStructInstance())
  {
    vm->pushString(arg.as.sInstance->def->name->chars());
    return 1;
  }
  if (arg.isNativeClassInstance() && arg.as.sClassInstance->klass)
  {
    vm->pushString(arg.as.sClassInstance->klass->name->chars());
    return 1;
  }
  vm->runtimeError("classname() argument is not a class/struct instance");
  return 0;
}

int native_int(Interpreter *vm, int argCount, Value *args)
{
  if (argCount != 1)
  {
    vm->runtimeError("int() expects exactly one argument");
    return 0;
  }

  const Value &arg = args[0];
  int intValue = 0;

  switch (arg.type)
  {
  case ValueType::INT:
    intValue = arg.as.integer;
    break;
  case ValueType::UINT:
    intValue = static_cast<int>(arg.as.unsignedInteger);
    break;
  case ValueType::FLOAT:
    intValue = static_cast<int>(arg.as.real);
    break;
  case ValueType::DOUBLE:
    intValue = static_cast<int>(arg.as.number);
    break;
  case ValueType::STRING:
  {
    const char *str = arg.asStringChars();
    intValue = std::strtol(str, nullptr, 10);
    break;
  }
  default:
    vm->runtimeError("int() cannot convert value of this type to int");
    return 0;
  }

  vm->push(vm->makeInt(intValue));
  return 1;
}

int native_real(Interpreter *vm, int argCount, Value *args)
{
  if (argCount != 1)
  {
    vm->runtimeError("real() expects exactly one argument");
    return 0;
  }

  const Value &arg = args[0];
  double floatValue = 0.0;

  switch (arg.type)
  {
  case ValueType::INT:
    floatValue = static_cast<double>(arg.as.integer);
    break;
  case ValueType::UINT:
    floatValue = static_cast<double>(arg.as.unsignedInteger);
    break;
  case ValueType::FLOAT:
    floatValue = arg.as.real;
    break;
  case ValueType::DOUBLE:
    floatValue = static_cast<double>(arg.as.number);
    break;
  case ValueType::STRING:
  {
    const char *str = arg.asStringChars();
    floatValue = std::strtod(str, nullptr);
    break;
  }
  default:
    vm->runtimeError("real() cannot convert value of this type to real");
    return 0;
  }

  vm->push(vm->makeDouble(static_cast<double>(floatValue)));
  return 1;
}

int native_format(Interpreter *vm, int argCount, Value *args)
{
  if (argCount < 1 || args[0].type != ValueType::STRING)
  {
    vm->runtimeError("format expects string as first argument");
    return 0;
  }

  const char *fmt = args[0].asStringChars();
  std::string result;
  int argIndex = 1;

  for (int i = 0; fmt[i] != '\0'; i++)
  {
    if (fmt[i] == '{' && fmt[i + 1] == '}')
    {
      if (argIndex < argCount)
      {
        valueToString(args[argIndex++], result);
      }
      i++;
    }
    else
    {
      result += fmt[i];
    }
  }

  vm->push(vm->makeString(result.c_str()));
  return 1;
}

int native_write(Interpreter *vm, int argCount, Value *args)
{
  if (argCount < 1 || args[0].type != ValueType::STRING)
  {
    vm->runtimeError("write expects string as first argument");
    return 0;
  }

  const char *fmt = args[0].asStringChars();
  std::string result;
  int argIndex = 1;

  for (int i = 0; fmt[i] != '\0'; i++)
  {
    if (fmt[i] == '{' && fmt[i + 1] == '}')
    {
      if (argIndex < argCount)
      {
        valueToString(args[argIndex++], result);
      }
      i++;
    }
    else
    {
      result += fmt[i];
    }
  }

  OsPrintf("%s", result.c_str());
  return 0;
}

int native_input(Interpreter *vm, int argCount, Value *args)
{
  if (argCount > 0 && args[0].isString())
  {
    OsPrintf("%s", args[0].asStringChars());
    std::cout.flush();
  }

  std::string line;
  if (std::getline(std::cin, line))
  {
    // Normalize CRLF on Windows consoles/pipes.
    if (!line.empty() && line.back() == '\r')
    {
      line.pop_back();
    }

    vm->push(vm->makeString(line.c_str()));
    return 1;
  }

  return 0;
}

int native_gc(Interpreter *vm, int argCount, Value *args)
{
  vm->runGC();
  return 0;
}


int native_ticks(Interpreter *vm, int argCount, Value *args)
{
  if (argCount != 1 || !args[0].isNumber())
  {
    vm->runtimeError("ticks expects double as argument");
    return 0;
  }
  vm->update(args[0].asNumber());
  return 0;
}



void Interpreter::registerBase()
{
  NativeClassDef *gcPersistentProbe = registerNativeClass("GcPersistentProbe", gc_persistent_probe_ctor, gc_probe_dtor, 1, true);
  addNativeMethod(gcPersistentProbe, "id", gc_probe_id);
  addNativeMethod(gcPersistentProbe, "isPersistent", gc_probe_is_persistent);
  addNativeMethod(gcPersistentProbe, "destroyPayload", gc_probe_destroy_payload);
  addNativeMethod(gcPersistentProbe, "isPayloadAlive", gc_probe_is_payload_alive);

  NativeClassDef *gcEphemeralProbe = registerNativeClass("GcEphemeralProbe", gc_ephemeral_probe_ctor, gc_probe_dtor, 1, false);
  addNativeMethod(gcEphemeralProbe, "id", gc_probe_id);
  addNativeMethod(gcEphemeralProbe, "isPersistent", gc_probe_is_persistent);
  addNativeMethod(gcEphemeralProbe, "destroyPayload", gc_probe_destroy_payload);
  addNativeMethod(gcEphemeralProbe, "isPayloadAlive", gc_probe_is_payload_alive);

  registerNative("format", native_format, -1);
  registerNative("write", native_write, -1);
  registerNative("input", native_input, -1);
  registerNative("print_stack", native_print_stack, -1);
  registerNative("ticks", native_ticks, 1);
  registerNative("_gc", native_gc, 0);
  registerNative("_gcProbeReset", native_gc_probe_reset, 0);
  registerNative("_gcProbePersistentCtorCount", native_gc_probe_persistent_ctor_count, 0);
  registerNative("_gcProbePersistentDtorCount", native_gc_probe_persistent_dtor_count, 0);
  registerNative("_gcProbeEphemeralCtorCount", native_gc_probe_ephemeral_ctor_count, 0);
  registerNative("_gcProbeEphemeralDtorCount", native_gc_probe_ephemeral_dtor_count, 0);
  registerNative("_gcProbePersistentManualPayloadCount", native_gc_probe_persistent_manual_payload_count, 0);
  registerNative("_gcProbePersistentAutoPayloadCount", native_gc_probe_persistent_auto_payload_count, 0);
  registerNative("_gcProbeEphemeralManualPayloadCount", native_gc_probe_ephemeral_manual_payload_count, 0);
  registerNative("_gcProbeEphemeralAutoPayloadCount", native_gc_probe_ephemeral_auto_payload_count, 0);
  registerNative("str", native_string, 1);
  registerNative("int", native_int, 1);
  registerNative("real", native_real, 1);
  registerNative("classname", native_classname, 1);
  registerNative("typeid", native_typeid, 1);
}

void Interpreter::registerAll()
{
  registerBase();
  registerArray();

#ifdef BU_ENABLE_MATH
  registerMath();
#endif

#ifdef BU_ENABLE_OS

  registerOS();

#endif

#ifdef BU_ENABLE_PATH

  registerPath();

#endif

#ifdef BU_ENABLE_TIME

  registerTime();

#endif

#ifdef BU_ENABLE_FILE_IO
  registerFS();
  registerFile();
#endif

#ifdef BU_ENABLE_JSON
  registerJSON();
#endif

#ifdef BU_ENABLE_REGEX
  registerRegex();
#endif

#ifdef BU_ENABLE_ZIP
  registerZip();
#endif

#ifdef BU_ENABLE_SOCKETS
  registerSocket();
#endif
}
