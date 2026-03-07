
#include "interpreter.hpp"
#include "platform.hpp"
#include "utils.hpp"
#include <string>

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
    printf("%s", args[0].asStringChars());
  }

  char buffer[1024];
  if (fgets(buffer, sizeof(buffer), stdin) != NULL)
  {
    size_t length = strlen(buffer);

    // Remove o \n do final
    if (length > 0 && buffer[length - 1] == '\n')
    {
      buffer[length - 1] = '\0';
    }
    vm->push(vm->makeString(buffer));
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
  registerNative("format", native_format, -1);
  registerNative("write", native_write, -1);
  registerNative("input", native_input, -1);
  registerNative("print_stack", native_print_stack, -1);
  registerNative("ticks", native_ticks, 1);
  registerNative("_gc", native_gc, 0);
  registerNative("str", native_string, 1);
  registerNative("int", native_int, 1);
  registerNative("real", native_real, 1);
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
