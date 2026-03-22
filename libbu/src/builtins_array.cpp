#include "interpreter.hpp"
#include <cstdlib>
#include <cstring>

namespace
{
static constexpr const char *kClassUint8 = "Uint8Array";
static constexpr const char *kClassInt16 = "Int16Array";
static constexpr const char *kClassUint16 = "Uint16Array";
static constexpr const char *kClassInt32 = "Int32Array";
static constexpr const char *kClassUint32 = "Uint32Array";
static constexpr const char *kClassFloat32 = "Float32Array";
static constexpr const char *kClassFloat64 = "Float64Array";

struct BuiltinTypedArrayData
{
  BufferType type;
  int count;
  int capacity;
  int elementSize;
  uint8 *data;
};

static int element_size(BufferType type)
{
  switch (type)
  {
  case BufferType::UINT8:
    return 1;
  case BufferType::INT16:
  case BufferType::UINT16:
    return 2;
  case BufferType::INT32:
  case BufferType::UINT32:
  case BufferType::FLOAT:
    return 4;
  case BufferType::DOUBLE:
    return 8;
  default:
    return 1;
  }
}

static bool is_builtin_typedarray_class_name(const char *name)
{
  if (!name)
    return false;

  return (std::strcmp(name, kClassUint8) == 0) ||
         (std::strcmp(name, kClassInt16) == 0) ||
         (std::strcmp(name, kClassUint16) == 0) ||
         (std::strcmp(name, kClassInt32) == 0) ||
         (std::strcmp(name, kClassUint32) == 0) ||
         (std::strcmp(name, kClassFloat32) == 0) ||
         (std::strcmp(name, kClassFloat64) == 0);
}

static BuiltinTypedArrayData *as_typed_array(void *instance)
{
  return (BuiltinTypedArrayData *)instance;
}

static bool get_builtin_typedarray(const Value &value, BuiltinTypedArrayData **out)
{
  if (!value.isNativeClassInstance())
    return false;

  NativeClassInstance *inst = value.asNativeClassInstance();
  if (!inst || !inst->klass || !inst->klass->name)
    return false;

  const char *name = inst->klass->name->chars();
  if (!is_builtin_typedarray_class_name(name))
    return false;

  if (!inst->userData)
    return false;

  *out = as_typed_array(inst->userData);
  return true;
}

static double read_buffer_number(const BufferInstance *buf, int index)
{
  const uint8 *ptr = buf->data + ((size_t)index * (size_t)buf->elementSize);
  switch (buf->type)
  {
  case BufferType::UINT8:
    return (double)(*ptr);
  case BufferType::INT16:
    return (double)(*(const int16 *)ptr);
  case BufferType::UINT16:
    return (double)(*(const uint16 *)ptr);
  case BufferType::INT32:
    return (double)(*(const int32 *)ptr);
  case BufferType::UINT32:
    return (double)(*(const uint32 *)ptr);
  case BufferType::FLOAT:
    return (double)(*(const float *)ptr);
  case BufferType::DOUBLE:
    return *(const double *)ptr;
  default:
    return 0.0;
  }
}

static double read_typed_number(const BuiltinTypedArrayData *ta, int index)
{
  const uint8 *ptr = ta->data + ((size_t)index * (size_t)ta->elementSize);
  switch (ta->type)
  {
  case BufferType::UINT8:
    return (double)(*ptr);
  case BufferType::INT16:
    return (double)(*(const int16 *)ptr);
  case BufferType::UINT16:
    return (double)(*(const uint16 *)ptr);
  case BufferType::INT32:
    return (double)(*(const int32 *)ptr);
  case BufferType::UINT32:
    return (double)(*(const uint32 *)ptr);
  case BufferType::FLOAT:
    return (double)(*(const float *)ptr);
  case BufferType::DOUBLE:
    return *(const double *)ptr;
  default:
    return 0.0;
  }
}

static void write_typed_number(BuiltinTypedArrayData *ta, int index, double value)
{
  uint8 *ptr = ta->data + ((size_t)index * (size_t)ta->elementSize);
  switch (ta->type)
  {
  case BufferType::UINT8:
    *ptr = (uint8)value;
    return;
  case BufferType::INT16:
    *(int16 *)ptr = (int16)value;
    return;
  case BufferType::UINT16:
    *(uint16 *)ptr = (uint16)value;
    return;
  case BufferType::INT32:
    *(int32 *)ptr = (int32)value;
    return;
  case BufferType::UINT32:
    *(uint32 *)ptr = (uint32)value;
    return;
  case BufferType::FLOAT:
    *(float *)ptr = (float)value;
    return;
  case BufferType::DOUBLE:
    *(double *)ptr = (double)value;
    return;
  default:
    return;
  }
}

static bool ensure_capacity(BuiltinTypedArrayData *ta, int needed)
{
  if (needed <= ta->capacity)
    return true;

  int newCap = (ta->capacity > 0) ? ta->capacity : 1;
  while (newCap < needed)
  {
    if (newCap > (1 << 29))
      return false;
    newCap *= 2;
  }

  const size_t bytes = (size_t)newCap * (size_t)ta->elementSize;
  void *newData = std::realloc(ta->data, bytes);
  if (!newData)
    return false;

  ta->data = (uint8 *)newData;
  ta->capacity = newCap;
  return true;
}

static bool append_number(BuiltinTypedArrayData *ta, double value)
{
  if (!ensure_capacity(ta, ta->count + 1))
    return false;
  write_typed_number(ta, ta->count, value);
  ta->count++;
  return true;
}

static bool append_value(BuiltinTypedArrayData *ta, const Value &v)
{
  if (!v.isNumber())
    return false;
  return append_number(ta, v.asNumber());
}

static bool append_array(BuiltinTypedArrayData *ta, const Value &v)
{
  if (!v.isArray())
    return false;

  ArrayInstance *arr = v.asArray();
  const int n = (int)arr->values.size();

  for (int i = 0; i < n; i++)
  {
    if (!arr->values[i].isNumber())
      return false;
  }

  if (!ensure_capacity(ta, ta->count + n))
    return false;

  for (int i = 0; i < n; i++)
  {
    write_typed_number(ta, ta->count++, arr->values[i].asNumber());
  }
  return true;
}

static bool append_buffer(BuiltinTypedArrayData *ta, const Value &v)
{
  if (!v.isBuffer())
    return false;

  BufferInstance *buf = v.asBuffer();
  const int n = buf->count;
  if (!ensure_capacity(ta, ta->count + n))
    return false;

  for (int i = 0; i < n; i++)
  {
    write_typed_number(ta, ta->count++, read_buffer_number(buf, i));
  }
  return true;
}

static bool append_typed(BuiltinTypedArrayData *ta, const Value &v)
{
  BuiltinTypedArrayData *src = nullptr;
  if (!get_builtin_typedarray(v, &src))
    return false;

  if (!ensure_capacity(ta, ta->count + src->count))
    return false;

  for (int i = 0; i < src->count; i++)
  {
    write_typed_number(ta, ta->count++, read_typed_number(src, i));
  }
  return true;
}

static int typed_add(Interpreter *vm, void *instance, int argCount, Value *args)
{
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount < 1)
  {
    vm->runtimeError("add() expects at least 1 argument");
    return 0;
  }

  bool ok = true;
  if (argCount == 1)
  {
    if (args[0].isArray())
      ok = append_array(ta, args[0]);
    else if (args[0].isBuffer())
      ok = append_buffer(ta, args[0]);
    else
    {
      BuiltinTypedArrayData *tmp = nullptr;
      if (get_builtin_typedarray(args[0], &tmp))
        ok = append_typed(ta, args[0]);
      else
        ok = append_value(ta, args[0]);
    }
  }
  else
  {
    for (int i = 0; i < argCount; i++)
    {
      if (!append_value(ta, args[i]))
      {
        ok = false;
        break;
      }
    }
  }

  if (!ok)
  {
    vm->runtimeError("add() failed: invalid value or out of memory");
    return 0;
  }

  vm->pushInt(ta->count);
  return 1;
}

static int typed_clear(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)args;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 0)
  {
    vm->runtimeError("clear() expects 0 arguments");
    return 0;
  }
  ta->count = 0;
  vm->pushInt(0);
  return 1;
}

static int typed_reserve(Interpreter *vm, void *instance, int argCount, Value *args)
{
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 1 || !args[0].isNumber())
  {
    vm->runtimeError("reserve() expects 1 numeric argument");
    return 0;
  }

  int n = (int)args[0].asNumber();
  if (n < 0)
  {
    vm->runtimeError("reserve() expects >= 0");
    return 0;
  }

  if (!ensure_capacity(ta, n))
  {
    vm->runtimeError("reserve() out of memory");
    return 0;
  }

  vm->pushInt(ta->capacity);
  return 1;
}

static int typed_pack(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)args;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 0)
  {
    vm->runtimeError("pack() expects 0 arguments");
    return 0;
  }

  if (ta->count <= 0)
  {
    if (ta->data)
    {
      std::free(ta->data);
      ta->data = nullptr;
    }
    ta->capacity = 0;
    vm->pushInt(0);
    return 1;
  }

  if (ta->capacity == ta->count)
  {
    vm->pushInt(ta->capacity);
    return 1;
  }

  const size_t bytes = (size_t)ta->count * (size_t)ta->elementSize;
  void *newData = std::realloc(ta->data, bytes);
  if (!newData)
  {
    vm->runtimeError("pack() out of memory");
    return 0;
  }

  ta->data = (uint8 *)newData;
  ta->capacity = ta->count;
  vm->pushInt(ta->capacity);
  return 1;
}

static int typed_length(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)args;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 0)
    return 0;
  vm->pushInt(ta->count);
  return 1;
}

static int typed_capacity(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)args;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 0)
    return 0;
  vm->pushInt(ta->capacity);
  return 1;
}

static int typed_byte_length(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)args;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 0)
    return 0;
  vm->pushInt(ta->count * ta->elementSize);
  return 1;
}

static int typed_byte_capacity(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)args;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 0)
    return 0;
  vm->pushInt(ta->capacity * ta->elementSize);
  return 1;
}

static int typed_ptr(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)args;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 0)
    return 0;
  vm->pushPointer(ta->data);
  return 1;
}

static int typed_get(Interpreter *vm, void *instance, int argCount, Value *args)
{
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 1 || !args[0].isNumber())
  {
    vm->runtimeError("get() expects 1 numeric index");
    return 0;
  }

  int idx = (int)args[0].asNumber();
  if (idx < 0 || idx >= ta->count)
  {
    vm->runtimeError("get() index [%d] out of bounds [%d)", idx, ta->count);
    return 0;
  }

  vm->pushDouble(read_typed_number(ta, idx));
  return 1;
}

static int typed_set(Interpreter *vm, void *instance, int argCount, Value *args)
{
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
  {
    vm->runtimeError("set() expects (index, value)");
    return 0;
  }

  int idx = (int)args[0].asNumber();
  if (idx < 0 || idx >= ta->count)
  {
    vm->runtimeError("set() index [%d] out of bounds [%d])", idx, ta->count);
    return 0;
  }

  write_typed_number(ta, idx, args[1].asNumber());
  vm->pushInt(idx);
  return 1;
}

static int typed_to_buffer(Interpreter *vm, void *instance, int argCount, Value *args)
{
  (void)args;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta || argCount != 0)
    return 0;

  Value out = vm->makeBuffer(ta->count, (int)ta->type);
  BufferInstance *buf = out.asBuffer();
  if (ta->count > 0 && ta->data && buf && buf->data)
  {
    std::memcpy(buf->data, ta->data, (size_t)ta->count * (size_t)ta->elementSize);
  }
  vm->push(out);
  return 1;
}

static Value typed_get_length_prop(Interpreter *vm, void *instance)
{
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  return vm->makeInt(ta ? ta->count : 0);
}

static Value typed_get_capacity_prop(Interpreter *vm, void *instance)
{
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  return vm->makeInt(ta ? ta->capacity : 0);
}

static Value typed_get_byte_length_prop(Interpreter *vm, void *instance)
{
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  return vm->makeInt(ta ? (ta->count * ta->elementSize) : 0);
}

static Value typed_get_ptr_prop(Interpreter *vm, void *instance)
{
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  return vm->makePointer(ta ? (void *)ta->data : nullptr);
}

static void typed_destructor(Interpreter *vm, void *instance)
{
  (void)vm;
  BuiltinTypedArrayData *ta = as_typed_array(instance);
  if (!ta)
    return;
  if (ta->data)
    std::free(ta->data);
  delete ta;
}

static void *typed_constructor(Interpreter *vm, int argCount, Value *args, BufferType type, const char *className)
{
  if (argCount != 1)
  {
    vm->runtimeError("%s expects 1 argument: size | array | buffer | typedarray", className);
    return nullptr;
  }

  BuiltinTypedArrayData *ta = new BuiltinTypedArrayData();
  ta->type = type;
  ta->count = 0;
  ta->capacity = 0;
  ta->elementSize = element_size(type);
  ta->data = nullptr;

  Value src = args[0];
  if (src.isNumber())
  {
    int cap = (int)src.asNumber();
    if (cap < 0)
    {
      vm->runtimeError("%s size must be >= 0", className);
      delete ta;
      return nullptr;
    }
    if (cap > 0)
    {
      ta->data = (uint8 *)std::malloc((size_t)cap * (size_t)ta->elementSize);
      if (!ta->data)
      {
        vm->runtimeError("%s out of memory", className);
        delete ta;
        return nullptr;
      }
    }
    ta->capacity = cap;
    ta->count = 0;
    return ta;
  }

  if (src.isArray())
  {
    if (!append_array(ta, src))
    {
      vm->runtimeError("%s array constructor expects only numeric elements", className);
      if (ta->data)
        std::free(ta->data);
      delete ta;
      return nullptr;
    }
    return ta;
  }

  if (src.isBuffer())
  {
    if (!append_buffer(ta, src))
    {
      vm->runtimeError("%s buffer constructor failed (out of memory)", className);
      if (ta->data)
        std::free(ta->data);
      delete ta;
      return nullptr;
    }
    return ta;
  }

  BuiltinTypedArrayData *tmp = nullptr;
  if (get_builtin_typedarray(src, &tmp))
  {
    if (!append_typed(ta, src))
    {
      vm->runtimeError("%s typedarray constructor failed (out of memory)", className);
      if (ta->data)
        std::free(ta->data);
      delete ta;
      return nullptr;
    }
    return ta;
  }

  vm->runtimeError("%s expects numeric size, array, buffer or typedarray", className);
  if (ta->data)
    std::free(ta->data);
  delete ta;
  return nullptr;
}

static void *ctor_uint8(Interpreter *vm, int argCount, Value *args)
{
  return typed_constructor(vm, argCount, args, BufferType::UINT8, kClassUint8);
}

static void *ctor_int16(Interpreter *vm, int argCount, Value *args)
{
  return typed_constructor(vm, argCount, args, BufferType::INT16, kClassInt16);
}

static void *ctor_uint16(Interpreter *vm, int argCount, Value *args)
{
  return typed_constructor(vm, argCount, args, BufferType::UINT16, kClassUint16);
}

static void *ctor_int32(Interpreter *vm, int argCount, Value *args)
{
  return typed_constructor(vm, argCount, args, BufferType::INT32, kClassInt32);
}

static void *ctor_uint32(Interpreter *vm, int argCount, Value *args)
{
  return typed_constructor(vm, argCount, args, BufferType::UINT32, kClassUint32);
}

static void *ctor_float32(Interpreter *vm, int argCount, Value *args)
{
  return typed_constructor(vm, argCount, args, BufferType::FLOAT, kClassFloat32);
}

static void *ctor_float64(Interpreter *vm, int argCount, Value *args)
{
  return typed_constructor(vm, argCount, args, BufferType::DOUBLE, kClassFloat64);
}

static void register_typed_class_api(Interpreter &vm, NativeClassDef *klass)
{
  vm.addNativeMethod(klass, "add", typed_add);
  vm.addNativeMethod(klass, "clear", typed_clear);
  vm.addNativeMethod(klass, "reserve", typed_reserve);
  vm.addNativeMethod(klass, "pack", typed_pack);
  vm.addNativeMethod(klass, "length", typed_length);
  vm.addNativeMethod(klass, "capacity", typed_capacity);
  vm.addNativeMethod(klass, "byteLength", typed_byte_length);
  vm.addNativeMethod(klass, "byteCapacity", typed_byte_capacity);
  vm.addNativeMethod(klass, "ptr", typed_ptr);
  vm.addNativeMethod(klass, "get", typed_get);
  vm.addNativeMethod(klass, "set", typed_set);
  vm.addNativeMethod(klass, "toBuffer", typed_to_buffer);

  vm.addNativeProperty(klass, "length", typed_get_length_prop, nullptr);
  vm.addNativeProperty(klass, "capacity", typed_get_capacity_prop, nullptr);
  vm.addNativeProperty(klass, "byteLength", typed_get_byte_length_prop, nullptr);
  vm.addNativeProperty(klass, "ptr", typed_get_ptr_prop, nullptr);
}
} // namespace

bool getBuiltinTypedArrayData(const Value &value, const void **outData)
{
  if (!outData)
    return false;
  BuiltinTypedArrayData *ta = nullptr;
  if (!get_builtin_typedarray(value, &ta) || !ta)
    return false;
  *outData = ta->data;
  return true;
}

void Interpreter::registerArray()
{
  addGlobal("TYPE_UINT8", makeInt((int)BufferType::UINT8));
  addGlobal("TYPE_INT16", makeInt((int)BufferType::INT16));
  addGlobal("TYPE_UINT16", makeInt((int)BufferType::UINT16));
  addGlobal("TYPE_INT32", makeInt((int)BufferType::INT32));
  addGlobal("TYPE_UINT32", makeInt((int)BufferType::UINT32));
  addGlobal("TYPE_FLOAT", makeInt((int)BufferType::FLOAT));
  addGlobal("TYPE_DOUBLE", makeInt((int)BufferType::DOUBLE));

  NativeClassDef *u8 = registerNativeClass(kClassUint8, ctor_uint8, typed_destructor, 1, false);
  NativeClassDef *i16 = registerNativeClass(kClassInt16, ctor_int16, typed_destructor, 1, false);
  NativeClassDef *u16 = registerNativeClass(kClassUint16, ctor_uint16, typed_destructor, 1, false);
  NativeClassDef *i32 = registerNativeClass(kClassInt32, ctor_int32, typed_destructor, 1, false);
  NativeClassDef *u32 = registerNativeClass(kClassUint32, ctor_uint32, typed_destructor, 1, false);
  NativeClassDef *f32 = registerNativeClass(kClassFloat32, ctor_float32, typed_destructor, 1, false);
  NativeClassDef *f64 = registerNativeClass(kClassFloat64, ctor_float64, typed_destructor, 1, false);

  register_typed_class_api(*this, u8);
  register_typed_class_api(*this, i16);
  register_typed_class_api(*this, u16);
  register_typed_class_api(*this, i32);
  register_typed_class_api(*this, u32);
  register_typed_class_api(*this, f32);
  register_typed_class_api(*this, f64);
}
