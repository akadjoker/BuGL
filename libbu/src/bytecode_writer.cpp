#include "interpreter.hpp"
#include "bytecode_format.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <limits>

namespace
{
class BytecodeWriter
{
public:
  explicit BytecodeWriter(FILE *file) : file_(file), ok_(true) {}

  bool ok() const { return ok_; }

  bool writeRaw(const void *data, size_t size)
  {
    if (!ok_)
    {
      return false;
    }

    if (size == 0)
    {
      return true;
    }

    if (!data)
    {
      ok_ = false;
      return false;
    }

    if (fwrite(data, 1, size, file_) != size)
    {
      ok_ = false;
      return false;
    }
    return true;
  }

  bool writeU8(uint8 value)
  {
    return writeRaw(&value, sizeof(value));
  }

  bool writeU16(uint16 value)
  {
    uint8 data[2];
    data[0] = (uint8)(value & 0xFFu);
    data[1] = (uint8)((value >> 8u) & 0xFFu);
    return writeRaw(data, sizeof(data));
  }

  bool writeU32(uint32 value)
  {
    uint8 data[4];
    data[0] = (uint8)(value & 0xFFu);
    data[1] = (uint8)((value >> 8u) & 0xFFu);
    data[2] = (uint8)((value >> 16u) & 0xFFu);
    data[3] = (uint8)((value >> 24u) & 0xFFu);
    return writeRaw(data, sizeof(data));
  }

  bool writeI32(int32 value)
  {
    return writeU32((uint32)value);
  }

  bool writeF32(float value)
  {
    uint32 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return writeU32(bits);
  }

  bool writeF64(double value)
  {
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));

    uint8 data[8];
    data[0] = (uint8)(bits & 0xFFu);
    data[1] = (uint8)((bits >> 8u) & 0xFFu);
    data[2] = (uint8)((bits >> 16u) & 0xFFu);
    data[3] = (uint8)((bits >> 24u) & 0xFFu);
    data[4] = (uint8)((bits >> 32u) & 0xFFu);
    data[5] = (uint8)((bits >> 40u) & 0xFFu);
    data[6] = (uint8)((bits >> 48u) & 0xFFu);
    data[7] = (uint8)((bits >> 56u) & 0xFFu);
    return writeRaw(data, sizeof(data));
  }

private:
  FILE *file_;
  bool ok_;
};

bool checkedU32(Interpreter *vm, size_t value, const char *what, uint32 *out)
{
  if (value > std::numeric_limits<uint32>::max())
  {
    vm->safetimeError("saveBytecode: %s is too large (%zu)", what, value);
    return false;
  }

  *out = (uint32)value;
  return true;
}

bool writeString(Interpreter *vm, BytecodeWriter &writer, String *value)
{
  if (!value)
  {
    return writer.writeU32(0);
  }

  uint32 len = 0;
  if (!checkedU32(vm, value->length(), "string length", &len))
  {
    return false;
  }

  if (!writer.writeU32(len))
  {
    return false;
  }

  if (len == 0)
  {
    return true;
  }
  return writer.writeRaw(value->chars(), len);
}

bool writeRequiredString(Interpreter *vm, BytecodeWriter &writer, String *value, const char *context)
{
  if (!value)
  {
    vm->safetimeError("saveBytecode: missing required string in %s", context);
    return false;
  }
  return writeString(vm, writer, value);
}

bool writeOptionalString(Interpreter *vm, BytecodeWriter &writer, String *value)
{
  if (!writer.writeU8(value ? 1 : 0))
  {
    return false;
  }

  if (!value)
  {
    return true;
  }
  return writeString(vm, writer, value);
}

bool writeValue(Interpreter *vm, BytecodeWriter &writer, const Value &value, const char *context)
{
  using BytecodeFormat::ConstantTag;

  switch (value.type)
  {
  case ValueType::NIL:
    return writer.writeU8((uint8)ConstantTag::NIL);

  case ValueType::BOOL:
    return writer.writeU8((uint8)ConstantTag::BOOL) && writer.writeU8(value.asBool() ? 1 : 0);

  case ValueType::BYTE:
    return writer.writeU8((uint8)ConstantTag::BYTE) && writer.writeU8(value.as.byte);

  case ValueType::INT:
    return writer.writeU8((uint8)ConstantTag::INT) && writer.writeI32(value.as.integer);

  case ValueType::UINT:
    return writer.writeU8((uint8)ConstantTag::UINT) && writer.writeU32(value.as.unsignedInteger);

  case ValueType::FLOAT:
    return writer.writeU8((uint8)ConstantTag::FLOAT) && writer.writeF32(value.as.real);

  case ValueType::DOUBLE:
    return writer.writeU8((uint8)ConstantTag::DOUBLE) && writer.writeF64(value.as.number);

  case ValueType::STRING:
    return writer.writeU8((uint8)ConstantTag::STRING) && writeString(vm, writer, value.as.string);

  case ValueType::FUNCTION:
    return writer.writeU8((uint8)ConstantTag::FUNCTION_REF) && writer.writeI32(value.as.integer);

  case ValueType::NATIVE:
    return writer.writeU8((uint8)ConstantTag::NATIVE_REF) && writer.writeI32(value.as.integer);

  case ValueType::NATIVEPROCESS:
    return writer.writeU8((uint8)ConstantTag::NATIVE_PROCESS_REF) && writer.writeI32(value.as.integer);

  case ValueType::PROCESS:
    return writer.writeU8((uint8)ConstantTag::PROCESS_REF) && writer.writeI32(value.as.integer);

  case ValueType::STRUCT:
    return writer.writeU8((uint8)ConstantTag::STRUCT_REF) && writer.writeI32(value.as.integer);

  case ValueType::CLASS:
    return writer.writeU8((uint8)ConstantTag::CLASS_REF) && writer.writeI32(value.as.integer);

  case ValueType::NATIVECLASS:
    return writer.writeU8((uint8)ConstantTag::NATIVE_CLASS_REF) && writer.writeI32(value.as.integer);

  case ValueType::NATIVESTRUCT:
    return writer.writeU8((uint8)ConstantTag::NATIVE_STRUCT_REF) && writer.writeI32(value.as.integer);

  case ValueType::MODULEREFERENCE:
    return writer.writeU8((uint8)ConstantTag::MODULE_REF) && writer.writeU32(value.as.unsignedInteger);

  default:
    vm->safetimeError("saveBytecode: unsupported value type %d in %s", (int)value.type, context);
    return false;
  }
}

uint32 computeIpOffset(const Function *func, const uint8 *ip)
{
  if (!func || !func->chunk || !func->chunk->code || !ip)
  {
    return 0xFFFFFFFFu;
  }

  const uint8 *base = func->chunk->code;
  if (ip < base)
  {
    return 0xFFFFFFFFu;
  }

  size_t offset = (size_t)(ip - base);
  if (offset > func->chunk->count || offset > std::numeric_limits<uint32>::max())
  {
    return 0xFFFFFFFFu;
  }

  return (uint32)offset;
}

bool writeChunk(Interpreter *vm, BytecodeWriter &writer, const Code *chunk, const char *ownerName)
{
  if (!chunk)
  {
    vm->safetimeError("saveBytecode: function '%s' has null chunk", ownerName);
    return false;
  }

  uint32 codeCount = 0;
  if (!checkedU32(vm, chunk->count, "chunk bytecode size", &codeCount))
  {
    return false;
  }

  if (!writer.writeU32(codeCount))
  {
    return false;
  }

  if (codeCount > 0)
  {
    if (!chunk->code || !chunk->lines)
    {
      vm->safetimeError("saveBytecode: function '%s' has incomplete chunk buffers", ownerName);
      return false;
    }

    if (!writer.writeRaw(chunk->code, codeCount))
    {
      return false;
    }
  }

  if (!writer.writeU32(codeCount))
  {
    return false;
  }

  for (uint32 i = 0; i < codeCount; ++i)
  {
    if (!writer.writeI32((int32)chunk->lines[i]))
    {
      return false;
    }
  }

  uint32 constantsCount = 0;
  if (!checkedU32(vm, chunk->constants.size(), "chunk constants size", &constantsCount))
  {
    return false;
  }

  if (!writer.writeU32(constantsCount))
  {
    return false;
  }

  for (uint32 i = 0; i < constantsCount; ++i)
  {
    if (!writeValue(vm, writer, chunk->constants[i], ownerName))
    {
      return false;
    }
  }

  return true;
}

bool writeFunctionRecord(Interpreter *vm, BytecodeWriter &writer, Function *func)
{
  if (!writer.writeU8(func ? 1 : 0))
  {
    return false;
  }

  if (!func)
  {
    return true;
  }

  if (!writer.writeI32((int32)func->index))
  {
    return false;
  }

  if (!writeOptionalString(vm, writer, func->name))
  {
    return false;
  }

  if (!writer.writeI32((int32)func->arity))
  {
    return false;
  }

  if (!writer.writeU8(func->hasReturn ? 1 : 0))
  {
    return false;
  }

  if (!writer.writeI32((int32)func->upvalueCount))
  {
    return false;
  }

  const char *name = func->name ? func->name->chars() : "<anonymous>";
  return writeChunk(vm, writer, func->chunk, name);
}

bool writeProcessRecord(Interpreter *vm, BytecodeWriter &writer, ProcessDef *proc)
{
  if (!writer.writeU8(proc ? 1 : 0))
  {
    return false;
  }

  if (!proc)
  {
    return true;
  }

  if (!writer.writeI32((int32)proc->index))
  {
    return false;
  }

  if (!writeOptionalString(vm, writer, proc->name))
  {
    return false;
  }

  static constexpr int32 kSerializedFiberCount = 1;
  if (!writer.writeI32(kSerializedFiberCount))
  {
    return false;
  }

  if (!writer.writeI32(kSerializedFiberCount))
  {
    return false;
  }

  const char *procName = proc->name ? proc->name->chars() : "<anonymous process>";

  uint32 argsCount = 0;
  if (!checkedU32(vm, proc->argsNames.size(), "process args size", &argsCount))
  {
    return false;
  }

  if (!writer.writeU32(argsCount))
  {
    return false;
  }

  if (argsCount > 0)
  {
    if (!writer.writeRaw(proc->argsNames.data(), argsCount))
    {
      return false;
    }
  }

  if (!writer.writeU32((uint32)MAX_PRIVATES))
  {
    return false;
  }

  for (int i = 0; i < MAX_PRIVATES; ++i)
  {
    if (!writeValue(vm, writer, proc->privates[i], "process private"))
    {
      return false;
    }
  }

  const int fiberCount = 1;
  if (!writer.writeU32((uint32)fiberCount))
  {
    return false;
  }

  for (int i = 0; i < fiberCount; ++i)
  {
    ProcessExec &fiber = *proc;

    if (!writer.writeU8((uint8)fiber.state))
    {
      return false;
    }

    if (!writer.writeF32(fiber.resumeTime))
    {
      return false;
    }

    const int frameCount = fiber.frameCount < 0 ? 0 : fiber.frameCount;
    if (frameCount > FRAMES_MAX)
    {
      vm->safetimeError("saveBytecode: process '%s' fiber %d has invalid frame count (%d)",
                        procName, i, frameCount);
      return false;
    }

    if (!writer.writeI32((int32)frameCount))
    {
      return false;
    }

    if (!writer.writeI32((int32)fiber.gosubTop))
    {
      return false;
    }

    if (!writer.writeI32((int32)fiber.tryDepth))
    {
      return false;
    }

    if (!writer.writeU32((uint32)frameCount))
    {
      return false;
    }

    for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
    {
      CallFrame &frame = fiber.frames[frameIndex];
      int32 functionIndex = frame.func ? (int32)frame.func->index : -1;
      const uint8 *frameIp = frame.ip;
      if (frame.func && !frameIp && frame.func->chunk)
      {
        frameIp = frame.func->chunk->code;
      }

      if (!writer.writeI32(functionIndex))
      {
        return false;
      }

      uint32 frameIpOffset = computeIpOffset(frame.func, frameIp);
      if (frame.func && frameIpOffset == 0xFFFFFFFFu)
      {
        vm->safetimeError("saveBytecode: process '%s' fiber %d frame %d has invalid instruction pointer",
                          procName, i, frameIndex);
        return false;
      }

      if (!writer.writeU32(frameIpOffset))
      {
        return false;
      }

      int32 slotOffset = -1;
      if (frame.slots && frame.slots >= fiber.stack && frame.slots <= fiber.stack + STACK_MAX)
      {
        slotOffset = (int32)(frame.slots - fiber.stack);
      }

      if (!writer.writeI32(slotOffset))
      {
        return false;
      }
    }

    Function *baseFunc = (frameCount > 0) ? fiber.frames[0].func : nullptr;
    const uint8 *fiberIp = fiber.ip;
    if (baseFunc && !fiberIp && baseFunc->chunk)
    {
      fiberIp = baseFunc->chunk->code;
    }

    uint32 fiberIpOffset = computeIpOffset(baseFunc, fiberIp);
    if (!writer.writeU32(fiberIpOffset))
    {
      return false;
    }

    int32 stackSize = -1;
    if (fiber.stackTop && fiber.stackTop >= fiber.stack && fiber.stackTop <= fiber.stack + STACK_MAX)
    {
      stackSize = (int32)(fiber.stackTop - fiber.stack);
    }

    if (!writer.writeI32(stackSize))
    {
      return false;
    }
  }

  return true;
}

bool writeStructRecord(Interpreter *vm, BytecodeWriter &writer, StructDef *def)
{
  if (!writer.writeU8(def ? 1 : 0))
  {
    return false;
  }

  if (!def)
  {
    return true;
  }

  if (!writer.writeI32((int32)def->index))
  {
    return false;
  }

  if (!writeOptionalString(vm, writer, def->name))
  {
    return false;
  }

  if (!writer.writeU8((uint8)def->argCount))
  {
    return false;
  }

  uint32 fieldsCount = 0;
  if (!checkedU32(vm, def->names.count, "struct field count", &fieldsCount))
  {
    return false;
  }

  if (!writer.writeU32(fieldsCount))
  {
    return false;
  }

  for (uint32 i = 0; i < fieldsCount; ++i)
  {
    String *name = def->names.entries[i].key;
    uint8 index = def->names.entries[i].value;

    if (!writeRequiredString(vm, writer, name, "struct field name"))
    {
      return false;
    }

    if (!writer.writeU8(index))
    {
      return false;
    }
  }

  return true;
}

bool writeClassRecord(Interpreter *vm, BytecodeWriter &writer, ClassDef *klass)
{
  if (!writer.writeU8(klass ? 1 : 0))
  {
    return false;
  }

  if (!klass)
  {
    return true;
  }

  if (!writer.writeI32((int32)klass->index))
  {
    return false;
  }

  if (!writeOptionalString(vm, writer, klass->name))
  {
    return false;
  }

  if (!writeOptionalString(vm, writer, klass->parent))
  {
    return false;
  }

  if (!writer.writeU8(klass->inherited ? 1 : 0))
  {
    return false;
  }

  if (!writer.writeI32((int32)klass->fieldCount))
  {
    return false;
  }

  if (!writer.writeI32(klass->constructor ? (int32)klass->constructor->index : -1))
  {
    return false;
  }

  if (!writer.writeI32(klass->superclass ? (int32)klass->superclass->index : -1))
  {
    return false;
  }

  if (!writer.writeI32(klass->nativeSuperclass ? (int32)klass->nativeSuperclass->index : -1))
  {
    return false;
  }

  uint32 fieldsCount = 0;
  if (!checkedU32(vm, klass->fieldNames.count, "class field count", &fieldsCount))
  {
    return false;
  }

  if (!writer.writeU32(fieldsCount))
  {
    return false;
  }

  for (uint32 i = 0; i < fieldsCount; ++i)
  {
    String *fieldName = klass->fieldNames.entries[i].key;
    uint8 fieldIndex = klass->fieldNames.entries[i].value;

    if (!writeRequiredString(vm, writer, fieldName, "class field name"))
    {
      return false;
    }

    if (!writer.writeU8(fieldIndex))
    {
      return false;
    }
  }

  uint32 defaultsCount = 0;
  if (!checkedU32(vm, klass->fieldDefaults.size(), "class field defaults count", &defaultsCount))
  {
    return false;
  }

  if (!writer.writeU32(defaultsCount))
  {
    return false;
  }

  for (uint32 i = 0; i < defaultsCount; ++i)
  {
    if (!writeValue(vm, writer, klass->fieldDefaults[i], "class field default"))
    {
      return false;
    }
  }

  uint32 methodsCount = 0;
  if (!checkedU32(vm, klass->methods.count, "class methods count", &methodsCount))
  {
    return false;
  }

  if (!writer.writeU32(methodsCount))
  {
    return false;
  }

  for (uint32 i = 0; i < methodsCount; ++i)
  {
    String *methodName = klass->methods.entries[i].key;
    Function *method = klass->methods.entries[i].value;

    if (!writeRequiredString(vm, writer, methodName, "class method name"))
    {
      return false;
    }

    if (!writer.writeI32(method ? (int32)method->index : -1))
    {
      return false;
    }
  }

  return true;
}

bool writeModuleRecord(Interpreter *vm, BytecodeWriter &writer, ModuleDef *module)
{
  if (!writer.writeU8(module ? 1 : 0))
  {
    return false;
  }

  if (!module)
  {
    return true;
  }

  if (!writeOptionalString(vm, writer, module->getName()))
  {
    return false;
  }

  const char *moduleName = module->getName() ? module->getName()->chars() : "<anonymous module>";

  uint32 functionsCount = 0;
  if (!checkedU32(vm, module->functions.size(), "module function count", &functionsCount))
  {
    return false;
  }

  if (!writer.writeU32(functionsCount))
  {
    return false;
  }

  for (uint32 i = 0; i < functionsCount; ++i)
  {
    String *name = nullptr;
    if (!module->getFunctionName((uint16)i, &name))
    {
      vm->safetimeError("saveBytecode: module '%s' has no name for function id %u", moduleName, i);
      return false;
    }

    if (!writeOptionalString(vm, writer, name))
    {
      return false;
    }

    if (!writer.writeI32((int32)module->functions[i].arity))
    {
      return false;
    }
  }

  // Module constants stay private inside ModuleDef.
  // For v1 writer we only emit function metadata.
  if (!writer.writeU32(0u))
  {
    return false;
  }

  return true;
}

} // namespace

bool Interpreter::saveBytecode(const char *filename)
{
  if (!filename || filename[0] == '\0')
  {
    safetimeError("saveBytecode: invalid output path");
    return false;
  }

  std::string tempPath = std::string(filename) + ".tmp";

  FILE *file = fopen(tempPath.c_str(), "wb");
  if (!file)
  {
    safetimeError("saveBytecode: failed to open temporary file '%s' for writing", tempPath.c_str());
    return false;
  }

  BytecodeWriter writer(file);

  uint32 functionsCount = 0;
  uint32 processesCount = 0;
  uint32 structsCount = 0;
  uint32 classesCount = 0;
  uint32 globalsCount = 0;
  uint32 nativesCount = 0;
  uint32 nativeProcessesCount = 0;
  uint32 modulesCount = 0;

  bool ok = checkedU32(this, functions.size(), "function count", &functionsCount) &&
            checkedU32(this, processes.size(), "process count", &processesCount) &&
            checkedU32(this, structs.size(), "struct count", &structsCount) &&
            checkedU32(this, classes.size(), "class count", &classesCount) &&
            checkedU32(this, globalIndexToName_.size(), "global name count", &globalsCount) &&
            checkedU32(this, natives.size(), "native function count", &nativesCount) &&
            checkedU32(this, nativeProcesses.size(), "native process count", &nativeProcessesCount) &&
            checkedU32(this, modules.size(), "module count", &modulesCount);

  if (!ok)
  {
    fclose(file);
    std::remove(tempPath.c_str());
    return false;
  }

  uint32 sectionFlags = 0u;
  if (processesCount > 0) sectionFlags |= BytecodeFormat::HAS_PROCESSES;
  if (structsCount > 0) sectionFlags |= BytecodeFormat::HAS_STRUCTS;
  if (classesCount > 0) sectionFlags |= BytecodeFormat::HAS_CLASSES;
  if (globalsCount > 0) sectionFlags |= BytecodeFormat::HAS_GLOBAL_NAMES;

  ok = writer.writeRaw(BytecodeFormat::MAGIC, sizeof(BytecodeFormat::MAGIC)) &&
       writer.writeU16(BytecodeFormat::VERSION_MAJOR) &&
       writer.writeU16(BytecodeFormat::VERSION_MINOR) &&
       writer.writeU32(sectionFlags) &&
       writer.writeU32(functionsCount) &&
       writer.writeU32(processesCount) &&
       writer.writeU32(structsCount) &&
       writer.writeU32(classesCount) &&
       writer.writeU32(globalsCount) &&
       writer.writeU32(nativesCount) &&
       writer.writeU32(nativeProcessesCount) &&
       writer.writeU32(modulesCount);

  if (!ok)
  {
    safetimeError("saveBytecode: failed while writing file header");
    fclose(file);
    std::remove(tempPath.c_str());
    return false;
  }

  for (uint32 i = 0; i < functionsCount; ++i)
  {
    if (!writeFunctionRecord(this, writer, functions[i]))
    {
      ok = false;
      break;
    }
  }

  for (uint32 i = 0; ok && i < processesCount; ++i)
  {
    if (!writeProcessRecord(this, writer, processes[i]))
    {
      ok = false;
      break;
    }
  }

  for (uint32 i = 0; ok && i < structsCount; ++i)
  {
    if (!writeStructRecord(this, writer, structs[i]))
    {
      ok = false;
      break;
    }
  }

  for (uint32 i = 0; ok && i < classesCount; ++i)
  {
    if (!writeClassRecord(this, writer, classes[i]))
    {
      ok = false;
      break;
    }
  }

  if (ok)
  {
    for (uint32 i = 0; i < globalsCount; ++i)
    {
      if (!writeOptionalString(this, writer, globalIndexToName_[i]))
      {
        ok = false;
        break;
      }
    }
  }

  if (ok)
  {
    for (uint32 i = 0; i < nativesCount; ++i)
    {
      NativeDef &native = natives[i];
      if (!writer.writeI32((int32)native.index) ||
          !writeOptionalString(this, writer, native.name) ||
          !writer.writeI32((int32)native.arity))
      {
        ok = false;
        break;
      }
    }
  }

  if (ok)
  {
    for (uint32 i = 0; i < nativeProcessesCount; ++i)
    {
      NativeProcessDef &nativeProc = nativeProcesses[i];
      if (!writer.writeI32((int32)nativeProc.index) ||
          !writeOptionalString(this, writer, nativeProc.name) ||
          !writer.writeI32((int32)nativeProc.arity))
      {
        ok = false;
        break;
      }
    }
  }

  for (uint32 i = 0; ok && i < modulesCount; ++i)
  {
    if (!writeModuleRecord(this, writer, modules[i]))
    {
      ok = false;
      break;
    }
  }

  if (!ok || !writer.ok())
  {
    safetimeError("saveBytecode: failed to serialize '%s'", filename);
    fclose(file);
    std::remove(tempPath.c_str());
    return false;
  }

  if (fflush(file) != 0)
  {
    safetimeError("saveBytecode: failed to flush '%s'", filename);
    fclose(file);
    std::remove(tempPath.c_str());
    return false;
  }

  if (fclose(file) != 0)
  {
    safetimeError("saveBytecode: failed to close '%s'", filename);
    std::remove(tempPath.c_str());
    return false;
  }

  if (std::rename(tempPath.c_str(), filename) != 0)
  {
#ifdef OS_WINDOWS
    std::remove(filename);
    if (std::rename(tempPath.c_str(), filename) == 0)
    {
      return true;
    }
#endif
    safetimeError("saveBytecode: failed to replace '%s' with temporary file", filename);
    std::remove(tempPath.c_str());
    return false;
  }

  return true;
}

bool Interpreter::compileToBytecode(const char *source, const char *filename, bool dump)
{
  if (!compile(source, dump))
  {
    return false;
  }
  return saveBytecode(filename);
}
