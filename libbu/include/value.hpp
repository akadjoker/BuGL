#pragma once
#include "config.hpp"
#include "string.hpp"
#include "pool.hpp"

struct StructInstance;
struct ArrayInstance;
struct BufferInstance;
struct MapInstance;
struct ClassInstance;
struct NativeClassInstance;
struct NativeStructInstance;
struct Closure;
struct Process;

enum class ValueType : uint8
{
  NIL,
  BOOL,
  CHAR,
  BYTE,
  INT,
  UINT,
  LONG,
  ULONG,
  FLOAT,
  DOUBLE,
  STRING,
  ARRAY,
  MAP,
  BUFFER,
  STRUCT,
  STRUCTINSTANCE,
  FUNCTION,
  NATIVE,
  NATIVECLASS,
  NATIVECLASSINSTANCE,
  NATIVESTRUCT,
  NATIVESTRUCTINSTANCE,
  NATIVEPROCESS,
  CLASS,
  CLASSINSTANCE,
  PROCESS,
  PROCESS_INSTANCE,
  POINTER,
  MODULEREFERENCE,
  CLOSURE,
};

struct Value
{
  ValueType type;
  union
  {
    bool boolean;
    uint8 byte;
    int integer;

    float real;
    double number;
    String *string;

    uint32 unsignedInteger;
    StructInstance *sInstance;
    ArrayInstance *array;
    BufferInstance *buffer;
    MapInstance *map;
    ClassInstance *sClass;
    NativeClassInstance *sClassInstance;
    NativeStructInstance *sNativeStruct;
    Closure *closure;
    Process *process;
    void *pointer;

  } as;

  Value();
  Value(const Value &other) = default;
  Value(Value &&other) noexcept = default;
  Value &operator=(const Value &other) = default;
  Value &operator=(Value &&other) noexcept = default;

  // Type checks
  FORCE_INLINE bool isNumber() const { return ((type == ValueType::INT) || (type == ValueType::DOUBLE) || (type == ValueType::BYTE) || (type == ValueType::FLOAT) || (type == ValueType::UINT)); }
  FORCE_INLINE bool isNil() const { return type == ValueType::NIL; }
  FORCE_INLINE bool isBool() const { return type == ValueType::BOOL; }
  FORCE_INLINE bool isInt() const { return type == ValueType::INT; }
  FORCE_INLINE bool isByte() const { return type == ValueType::BYTE; }
  FORCE_INLINE bool isDouble() const { return type == ValueType::DOUBLE; }
  FORCE_INLINE bool isFloat() const { return type == ValueType::FLOAT; }
  FORCE_INLINE bool isUInt() const { return type == ValueType::UINT; }
  FORCE_INLINE bool isString() const { return type == ValueType::STRING; }
  FORCE_INLINE bool isFunction() const { return type == ValueType::FUNCTION; }
  FORCE_INLINE bool isNativeProcess() const { return type == ValueType::NATIVEPROCESS; }
  FORCE_INLINE bool isNative() const { return type == ValueType::NATIVE; }
  FORCE_INLINE bool isNativeClass() const { return type == ValueType::NATIVECLASS; }
  FORCE_INLINE bool isProcess() const { return type == ValueType::PROCESS; }
  FORCE_INLINE bool isProcessInstance() const { return type == ValueType::PROCESS_INSTANCE; }
  FORCE_INLINE bool isStruct() const { return type == ValueType::STRUCT; }
  FORCE_INLINE bool isStructInstance() const { return type == ValueType::STRUCTINSTANCE; }
  FORCE_INLINE bool isMap() const { return type == ValueType::MAP; }
  FORCE_INLINE bool isArray() const { return type == ValueType::ARRAY; }
  FORCE_INLINE bool isBuffer() const { return type == ValueType::BUFFER; }
  FORCE_INLINE bool isClass() const { return type == ValueType::CLASS; }
  FORCE_INLINE bool isClassInstance() const { return type == ValueType::CLASSINSTANCE; }
  FORCE_INLINE bool isNativeClassInstance() const { return type == ValueType::NATIVECLASSINSTANCE; }
  FORCE_INLINE bool isPointer() const { return type == ValueType::POINTER; }
  FORCE_INLINE bool isNativeStruct() const { return type == ValueType::NATIVESTRUCT; }
  FORCE_INLINE bool isNativeStructInstance() const { return type == ValueType::NATIVESTRUCTINSTANCE; }
  FORCE_INLINE bool isModuleRef() const { return type == ValueType::MODULEREFERENCE; }
  FORCE_INLINE bool isClosure() const { return type == ValueType::CLOSURE; }

  FORCE_INLINE bool isObject() const { return (isBuffer() || isMap() || isArray() || isClassInstance() || isStructInstance() || isNativeClassInstance() || isNativeStructInstance() || isClosure()); }

  // Conversions

  FORCE_INLINE const char *asStringChars() const { return as.string->chars(); }
  FORCE_INLINE String *asString() const { return as.string; }
  FORCE_INLINE int asFunctionId() const { return as.integer; }
  FORCE_INLINE int asNativeId() const { return as.integer; }
  FORCE_INLINE int asProcessId() const { return as.integer; }
  FORCE_INLINE int asNativeProcessId() const { return as.integer; }
  FORCE_INLINE Process *asProcess() const { return as.process; }

  FORCE_INLINE Closure * asClosure() const
  {
    return as.closure;
  }
  

  FORCE_INLINE int asStructId() const
  {
    return as.integer;
  }

  FORCE_INLINE int asClassId() const
  {
    return as.integer;
  }

  FORCE_INLINE int asClassNativeId() const
  {
    return as.integer;
  }

  FORCE_INLINE void *asPointer() const
  {
#ifdef DEBUG
    if (type != ValueType::POINTER)
    {
      Error("Cannot convert to pointer!");
    }
#endif
    return as.pointer;
  }

  FORCE_INLINE int asNativeStructId() const
  {
    return as.integer;
  }

  FORCE_INLINE StructInstance *asStructInstance() const
  {
    return as.sInstance;
  }

  FORCE_INLINE ArrayInstance *asArray() const
  {
    return as.array;
  }

  FORCE_INLINE MapInstance *asMap() const
  {
    return as.map;
  }

  FORCE_INLINE BufferInstance *asBuffer() const
  {
    return as.buffer;
  }

  FORCE_INLINE NativeClassInstance *asNativeClassInstance() const
  {
    return as.sClassInstance;
  }

  FORCE_INLINE ClassInstance *asClassInstance() const
  {
    return as.sClass;
  }

  FORCE_INLINE NativeStructInstance *asNativeStructInstance() const
  {
    return as.sNativeStruct;
  }

  FORCE_INLINE uint32 asUInt() const
  {
    if (LIKELY(type == ValueType::UINT))
    {
      return as.unsignedInteger;
    }

    switch (type)
    {
    case ValueType::INT:
      return (uint32)as.integer;
    case ValueType::BYTE:
      return (uint32)as.byte;
    case ValueType::BOOL:
      return (uint32)as.boolean;
    case ValueType::FLOAT:
      return (uint32)as.real;
    case ValueType::DOUBLE:
      return (uint32)as.number;
    default:
#ifdef DEBUG
      Error("Cannot convert to uint!");
#endif
      return 0u;
    }
  }

  FORCE_INLINE uint8 asByte() const
  {
    if (LIKELY(type == ValueType::BYTE))
    {
      return as.byte;
    }

    switch (type)
    {
    case ValueType::INT:
      return (uint8)as.integer;
    case ValueType::UINT:
      return (uint8)as.unsignedInteger;
    case ValueType::BOOL:
      return (uint8)as.boolean;
    case ValueType::FLOAT:
      return (uint8)as.real;
    case ValueType::DOUBLE:
      return (uint8)as.number;
    default:
#ifdef DEBUG
      Error("Cannot convert to byte!");
#endif
      return 0;
    }
  }

  FORCE_INLINE int asInt() const
  {
    if (LIKELY(type == ValueType::INT))
    {
      return as.integer;
    }
    switch (type)
    {
    case ValueType::DOUBLE:
      return (int)as.number;
    case ValueType::FLOAT:
      return (int)as.real;
    case ValueType::BYTE:
      return (int)as.byte;
    case ValueType::UINT:
      return (int)as.unsignedInteger;
    case ValueType::BOOL:
      return (int)as.boolean;
    default:
#ifdef DEBUG
      Error("Cannot convert to int!");
#endif
      return 0;
    }
  }

  FORCE_INLINE float asFloat() const
  {
    if (LIKELY(type == ValueType::FLOAT))
    {
      return as.real;
    }
    switch (type)
    {
    case ValueType::DOUBLE:
      return (float)as.number;
    case ValueType::INT:
      return (float)as.integer;
    case ValueType::BYTE:
      return (float)as.byte;
    case ValueType::UINT:
      return (float)as.unsignedInteger;
    case ValueType::BOOL:
      return (float)as.boolean;
    default:
#ifdef DEBUG
      Error("Cannot convert to float!");
#endif
      return 0.0f;
    }
  }

  FORCE_INLINE double asDouble() const
  {

    if (LIKELY(type == ValueType::DOUBLE))
    {
      return as.number;
    }

    switch (type)
    {
    case ValueType::FLOAT:
      return (double)as.real;
    case ValueType::INT:
      return (double)as.integer;
    case ValueType::BYTE:
      return (double)as.byte;
    case ValueType::UINT:
      return (double)as.unsignedInteger;
    case ValueType::BOOL:
      return (double)as.boolean;
    default:
#ifdef DEBUG
      Error("Cannot convert to double!");
#endif
      return 0.0;
    }
  }

  FORCE_INLINE bool asBool() const
  {
    if (LIKELY(type == ValueType::BOOL))
    {
      return as.boolean;
    }

    // Qualquer número != 0 é true
    switch (type)
    {
    case ValueType::INT:
      return as.integer != 0;
    case ValueType::UINT:
      return as.unsignedInteger != 0;
    case ValueType::BYTE:
      return as.byte != 0;
    case ValueType::FLOAT:
      return as.real != 0.0f;
    case ValueType::DOUBLE:
      return as.number != 0.0;
    case ValueType::NIL:
      return false;
    default:
      return true; // Objects são truthy
    }
  }

  FORCE_INLINE double asNumber() const
  {

    if (LIKELY(type == ValueType::DOUBLE))
    {
      return as.number;
    }

    switch (type)
    {
    case ValueType::FLOAT:
      return (double)as.real;
    case ValueType::INT:
      return (double)as.integer;
    case ValueType::BYTE:
      return (double)as.byte;
    case ValueType::UINT:
      return (double)as.unsignedInteger;
    case ValueType::BOOL:
      return (double)as.boolean;
    default:
#ifdef DEBUG
      Error("Cannot convert to number!");
#endif
      return 0.0;
    }
  }
};

void printValue(const Value &value);
const char *valueTypeToString(ValueType type);
void printValueNl(const Value &value);

void valueToBuffer(const Value &v, char *out, size_t size);

static FORCE_INLINE bool valuesEqual(const Value &a, const Value &b)
{
  // Numbers: allow cross-type comparison (int == double)
  if (a.isNumber() && b.isNumber())
  {
    double da = a.asNumber();
    double db = b.asNumber();
    return da == db;
  }

  // Rest require exact type match
  if (a.type != b.type)
    return false;

  switch (a.type)
  {
  case ValueType::BOOL:
    return a.asBool() == b.asBool();
  
  case ValueType::NIL:
    return true;
  
  case ValueType::STRING:
    return compare_strings(a.asString(), b.asString());
  
  // Object types: compare by pointer (identity)
  case ValueType::ARRAY:
    return a.asArray() == b.asArray();
  
  case ValueType::MAP:
    return a.asMap() == b.asMap();
  
  case ValueType::BUFFER:
    return a.asBuffer() == b.asBuffer();
  
  case ValueType::CLASSINSTANCE:
    return a.asClassInstance() == b.asClassInstance();
  
  case ValueType::STRUCTINSTANCE:
    return a.asStructInstance() == b.asStructInstance();
  
  case ValueType::NATIVECLASSINSTANCE:
    return a.asNativeClassInstance() == b.asNativeClassInstance();
  
  case ValueType::NATIVESTRUCTINSTANCE:
    return a.asNativeStructInstance() == b.asNativeStructInstance();
  
  case ValueType::CLOSURE:
    return a.asClosure() == b.asClosure();
  
  case ValueType::PROCESS_INSTANCE:
    return a.asProcess() == b.asProcess();

  case ValueType::POINTER:
    return a.asPointer() == b.asPointer();

  default:
    // Other types (functions, classes, natives, etc.) don't support equality
    return false;
  }
}

static FORCE_INLINE bool isTruthy(const Value &value)
{
  switch (value.type)
  {
  case ValueType::NIL:
    return false;
  case ValueType::BOOL:
    return value.asBool();
  case ValueType::INT:
    return value.asInt() != 0;
  case ValueType::DOUBLE:
    return value.asDouble() != 0.0;
  case ValueType::BYTE:
    return value.asByte() != 0;
  case ValueType::FLOAT:
    return value.asFloat() != 0.0f;
  default:
    return true; // strings, functions são truthy
  }
}

static FORCE_INLINE bool isFalsey(Value value)
{
  return !isTruthy(value);
}