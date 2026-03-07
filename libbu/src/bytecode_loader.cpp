#include "interpreter.hpp"
#include "bytecode_format.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
static const uint32 kInvalidIpOffset = 0xFFFFFFFFu;

class BytecodeReader
{
public:
  BytecodeReader()
      : file_(nullptr), memory_(nullptr), memorySize_(0), memoryOffset_(0), ok_(true) {}

  explicit BytecodeReader(FILE *file)
      : file_(file), memory_(nullptr), memorySize_(0), memoryOffset_(0), ok_(true) {}

  BytecodeReader(const uint8 *memory, size_t memorySize)
      : file_(nullptr), memory_(memory), memorySize_(memorySize), memoryOffset_(0), ok_(true) {}

  bool ok() const { return ok_; }

  bool readRaw(void *data, size_t size)
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

    if (memory_)
    {
      if (memoryOffset_ + size > memorySize_)
      {
        ok_ = false;
        return false;
      }
      std::memcpy(data, memory_ + memoryOffset_, size);
      memoryOffset_ += size;
      return true;
    }

    if (!file_ || fread(data, 1, size, file_) != size)
    {
      ok_ = false;
      return false;
    }
    return true;
  }

  bool readU8(uint8 *out)
  {
    return out && readRaw(out, sizeof(*out));
  }

  bool readU16(uint16 *out)
  {
    if (!out)
    {
      ok_ = false;
      return false;
    }

    uint8 data[2];
    if (!readRaw(data, sizeof(data)))
    {
      return false;
    }

    *out = (uint16)data[0] | ((uint16)data[1] << 8u);
    return true;
  }

  bool readU32(uint32 *out)
  {
    if (!out)
    {
      ok_ = false;
      return false;
    }

    uint8 data[4];
    if (!readRaw(data, sizeof(data)))
    {
      return false;
    }

    *out = ((uint32)data[0]) |
           ((uint32)data[1] << 8u) |
           ((uint32)data[2] << 16u) |
           ((uint32)data[3] << 24u);
    return true;
  }

  bool readI32(int32 *out)
  {
    if (!out)
    {
      ok_ = false;
      return false;
    }

    uint32 value = 0;
    if (!readU32(&value))
    {
      return false;
    }

    *out = (int32)value;
    return true;
  }

  bool readF32(float *out)
  {
    if (!out)
    {
      ok_ = false;
      return false;
    }

    uint32 bits = 0;
    if (!readU32(&bits))
    {
      return false;
    }
    std::memcpy(out, &bits, sizeof(*out));
    return true;
  }

  bool readF64(double *out)
  {
    if (!out)
    {
      ok_ = false;
      return false;
    }

    uint8 data[8];
    if (!readRaw(data, sizeof(data)))
    {
      return false;
    }

    uint64_t bits = ((uint64_t)data[0]) |
                    ((uint64_t)data[1] << 8u) |
                    ((uint64_t)data[2] << 16u) |
                    ((uint64_t)data[3] << 24u) |
                    ((uint64_t)data[4] << 32u) |
                    ((uint64_t)data[5] << 40u) |
                    ((uint64_t)data[6] << 48u) |
                    ((uint64_t)data[7] << 56u);
    std::memcpy(out, &bits, sizeof(*out));
    return true;
  }

private:
  FILE *file_;
  const uint8 *memory_;
  size_t memorySize_;
  size_t memoryOffset_;
  bool ok_;
};

bool stringEquals(String *a, String *b)
{
  if (a == b)
  {
    return true;
  }
  if (!a || !b)
  {
    return false;
  }
  return compare_strings(a, b);
}

bool readString(Interpreter *vm, BytecodeReader &reader, String **out)
{
  if (!out)
  {
    vm->safetimeError("loadBytecode: invalid string output pointer");
    return false;
  }

  uint32 len = 0;
  if (!reader.readU32(&len))
  {
    vm->safetimeError("loadBytecode: failed to read string length");
    return false;
  }

  if (len == 0u)
  {
    *out = vm->createString("", 0);
    if (!*out)
    {
      vm->safetimeError("loadBytecode: failed to allocate empty string");
      return false;
    }
    return true;
  }

  std::string buffer;
  buffer.resize((size_t)len);
  if (!reader.readRaw(&buffer[0], (size_t)len))
  {
    vm->safetimeError("loadBytecode: failed to read string bytes");
    return false;
  }

  *out = vm->createString(buffer.data(), len);
  if (!*out)
  {
    vm->safetimeError("loadBytecode: failed to allocate string");
    return false;
  }
  return true;
}

bool readOptionalString(Interpreter *vm, BytecodeReader &reader, String **out)
{
  if (!out)
  {
    vm->safetimeError("loadBytecode: invalid optional string output pointer");
    return false;
  }

  uint8 hasValue = 0;
  if (!reader.readU8(&hasValue))
  {
    vm->safetimeError("loadBytecode: failed to read optional string flag");
    return false;
  }

  if (hasValue == 0)
  {
    *out = nullptr;
    return true;
  }

  if (hasValue != 1)
  {
    vm->safetimeError("loadBytecode: invalid optional string flag value (%u)", (unsigned)hasValue);
    return false;
  }

  return readString(vm, reader, out);
}

bool readValue(Interpreter *vm, BytecodeReader &reader, Value *out, const char *context)
{
  if (!out)
  {
    vm->safetimeError("loadBytecode: invalid value output pointer (%s)", context);
    return false;
  }

  uint8 rawTag = 0;
  if (!reader.readU8(&rawTag))
  {
    vm->safetimeError("loadBytecode: failed to read constant tag in %s", context);
    return false;
  }

  using BytecodeFormat::ConstantTag;
  const ConstantTag tag = (ConstantTag)rawTag;

  switch (tag)
  {
  case ConstantTag::NIL:
    *out = vm->makeNil();
    return true;

  case ConstantTag::BOOL:
  {
    uint8 b = 0;
    if (!reader.readU8(&b))
    {
      vm->safetimeError("loadBytecode: failed to read bool value in %s", context);
      return false;
    }
    *out = vm->makeBool(b != 0);
    return true;
  }

  case ConstantTag::BYTE:
  {
    uint8 value = 0;
    if (!reader.readU8(&value))
    {
      vm->safetimeError("loadBytecode: failed to read byte value in %s", context);
      return false;
    }
    *out = vm->makeByte(value);
    return true;
  }

  case ConstantTag::INT:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read int value in %s", context);
      return false;
    }
    *out = vm->makeInt(value);
    return true;
  }

  case ConstantTag::UINT:
  {
    uint32 value = 0;
    if (!reader.readU32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read uint value in %s", context);
      return false;
    }
    *out = vm->makeUInt(value);
    return true;
  }

  case ConstantTag::FLOAT:
  {
    float value = 0.0f;
    if (!reader.readF32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read float value in %s", context);
      return false;
    }
    *out = vm->makeFloat(value);
    return true;
  }

  case ConstantTag::DOUBLE:
  {
    double value = 0.0;
    if (!reader.readF64(&value))
    {
      vm->safetimeError("loadBytecode: failed to read double value in %s", context);
      return false;
    }
    *out = vm->makeDouble(value);
    return true;
  }

  case ConstantTag::STRING:
  {
    String *stringValue = nullptr;
    if (!readString(vm, reader, &stringValue))
    {
      vm->safetimeError("loadBytecode: failed to read string value in %s", context);
      return false;
    }
    *out = vm->makeString(stringValue);
    return true;
  }

  case ConstantTag::FUNCTION_REF:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read function ref in %s", context);
      return false;
    }
    *out = vm->makeFunction(value);
    return true;
  }

  case ConstantTag::NATIVE_REF:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read native ref in %s", context);
      return false;
    }
    *out = vm->makeNative(value);
    return true;
  }

  case ConstantTag::NATIVE_PROCESS_REF:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read native process ref in %s", context);
      return false;
    }
    *out = vm->makeNativeProcess(value);
    return true;
  }

  case ConstantTag::PROCESS_REF:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read process ref in %s", context);
      return false;
    }
    *out = vm->makeProcess(value);
    return true;
  }

  case ConstantTag::STRUCT_REF:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read struct ref in %s", context);
      return false;
    }
    *out = vm->makeStruct(value);
    return true;
  }

  case ConstantTag::CLASS_REF:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read class ref in %s", context);
      return false;
    }
    *out = vm->makeClass(value);
    return true;
  }

  case ConstantTag::NATIVE_CLASS_REF:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read native class ref in %s", context);
      return false;
    }
    *out = vm->makeNativeClass(value);
    return true;
  }

  case ConstantTag::NATIVE_STRUCT_REF:
  {
    int32 value = 0;
    if (!reader.readI32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read native struct ref in %s", context);
      return false;
    }
    *out = vm->makeNativeStruct(value);
    return true;
  }

  case ConstantTag::MODULE_REF:
  {
    uint32 value = 0;
    if (!reader.readU32(&value))
    {
      vm->safetimeError("loadBytecode: failed to read module ref in %s", context);
      return false;
    }
    out->type = ValueType::MODULEREFERENCE;
    out->as.unsignedInteger = value;
    return true;
  }

  default:
    vm->safetimeError("loadBytecode: unsupported constant tag %u in %s", (unsigned)rawTag, context);
    return false;
  }
}

bool readChunk(Interpreter *vm, BytecodeReader &reader, Code **outChunk, const char *ownerName)
{
  if (!outChunk)
  {
    vm->safetimeError("loadBytecode: invalid chunk output pointer for '%s'", ownerName);
    return false;
  }

  uint32 codeCount = 0;
  if (!reader.readU32(&codeCount))
  {
    vm->safetimeError("loadBytecode: failed to read code size for '%s'", ownerName);
    return false;
  }

  size_t capacity = codeCount > 0 ? (size_t)codeCount : (size_t)16;
  Code *chunk = new Code(capacity);

  if (codeCount > 0)
  {
    if (!reader.readRaw(chunk->code, (size_t)codeCount))
    {
      vm->safetimeError("loadBytecode: failed to read bytecode for '%s'", ownerName);
      chunk->clear();
      delete chunk;
      return false;
    }
  }
  chunk->count = (size_t)codeCount;

  uint32 linesCount = 0;
  if (!reader.readU32(&linesCount))
  {
    vm->safetimeError("loadBytecode: failed to read line table size for '%s'", ownerName);
    chunk->clear();
    delete chunk;
    return false;
  }

  if (linesCount != codeCount)
  {
    vm->safetimeError("loadBytecode: line table mismatch for '%s' (%u != %u)",
                      ownerName, linesCount, codeCount);
    chunk->clear();
    delete chunk;
    return false;
  }

  for (uint32 i = 0; i < linesCount; ++i)
  {
    int32 line = 0;
    if (!reader.readI32(&line))
    {
      vm->safetimeError("loadBytecode: failed to read line entry %u for '%s'", i, ownerName);
      chunk->clear();
      delete chunk;
      return false;
    }
    chunk->lines[i] = (int)line;
  }

  uint32 constantsCount = 0;
  if (!reader.readU32(&constantsCount))
  {
    vm->safetimeError("loadBytecode: failed to read constants size for '%s'", ownerName);
    chunk->clear();
    delete chunk;
    return false;
  }

  chunk->constants.reserve(constantsCount);
  for (uint32 i = 0; i < constantsCount; ++i)
  {
    Value value;
    if (!readValue(vm, reader, &value, ownerName))
    {
      chunk->clear();
      delete chunk;
      return false;
    }
    chunk->constants.push(value);
  }

  *outChunk = chunk;
  return true;
}

bool readFunctionRecord(Interpreter *vm,
                        BytecodeReader &reader,
                        uint32 slotIndex,
                        Vector<Function *> &functions,
                        HashMap<String *, Function *, StringHasher, StringEq> &functionsMap)
{
  uint8 present = 0;
  if (!reader.readU8(&present))
  {
    vm->safetimeError("loadBytecode: failed to read function presence at slot %u", slotIndex);
    return false;
  }

  if (present == 0)
  {
    functions.push(nullptr);
    return true;
  }

  if (present != 1)
  {
    vm->safetimeError("loadBytecode: invalid function presence flag at slot %u", slotIndex);
    return false;
  }

  int32 index = 0;
  if (!reader.readI32(&index))
  {
    vm->safetimeError("loadBytecode: failed to read function index at slot %u", slotIndex);
    return false;
  }

  if (index < 0 || (uint32)index != slotIndex)
  {
    vm->safetimeError("loadBytecode: function index mismatch at slot %u (got %d)", slotIndex, index);
    return false;
  }

  String *name = nullptr;
  if (!readOptionalString(vm, reader, &name))
  {
    vm->safetimeError("loadBytecode: failed to read function name at slot %u", slotIndex);
    return false;
  }

  int32 arity = 0;
  if (!reader.readI32(&arity))
  {
    vm->safetimeError("loadBytecode: failed to read function arity at slot %u", slotIndex);
    return false;
  }

  uint8 hasReturn = 0;
  if (!reader.readU8(&hasReturn))
  {
    vm->safetimeError("loadBytecode: failed to read function return flag at slot %u", slotIndex);
    return false;
  }

  int32 upvalueCount = 0;
  if (!reader.readI32(&upvalueCount))
  {
    vm->safetimeError("loadBytecode: failed to read function upvalue count at slot %u", slotIndex);
    return false;
  }

  const char *ownerName = name ? name->chars() : "<anonymous>";
  Code *chunk = nullptr;
  if (!readChunk(vm, reader, &chunk, ownerName))
  {
    return false;
  }

  Function *func = new Function();
  func->index = index;
  func->arity = arity;
  func->name = name;
  func->hasReturn = hasReturn != 0;
  func->upvalueCount = upvalueCount;
  func->chunk = chunk;

  functions.push(func);
  if (name)
  {
    functionsMap.set(name, func);
  }
  return true;
}

bool resolveFunctionByIndex(Interpreter *vm,
                            const Vector<Function *> &functions,
                            int32 index,
                            const char *context,
                            Function **out)
{
  if (!out)
  {
    vm->safetimeError("loadBytecode: invalid function output pointer in %s", context);
    return false;
  }

  if (index < 0)
  {
    *out = nullptr;
    return true;
  }

  if ((size_t)index >= functions.size())
  {
    vm->safetimeError("loadBytecode: invalid function index %d in %s", index, context);
    return false;
  }

  Function *func = functions[(size_t)index];
  if (!func)
  {
    vm->safetimeError("loadBytecode: null function reference %d in %s", index, context);
    return false;
  }

  *out = func;
  return true;
}

bool readProcessRecord(Interpreter *vm,
                       BytecodeReader &reader,
                       uint32 slotIndex,
                       const Vector<Function *> &functions,
                       Vector<ProcessDef *> &processes,
                       HashMap<String *, ProcessDef *, StringHasher, StringEq> &processesMap)
{
  uint8 present = 0;
  if (!reader.readU8(&present))
  {
    vm->safetimeError("loadBytecode: failed to read process presence at slot %u", slotIndex);
    return false;
  }

  if (present == 0)
  {
    processes.push(nullptr);
    return true;
  }

  if (present != 1)
  {
    vm->safetimeError("loadBytecode: invalid process presence flag at slot %u", slotIndex);
    return false;
  }

  ProcessDef *proc = new ProcessDef();
  proc->index = -1;
  proc->name = nullptr;

  int32 index = 0;
  if (!reader.readI32(&index))
  {
    vm->safetimeError("loadBytecode: failed to read process index at slot %u", slotIndex);
    delete proc;
    return false;
  }

  if (index < 0 || (uint32)index != slotIndex)
  {
    vm->safetimeError("loadBytecode: process index mismatch at slot %u (got %d)", slotIndex, index);
    delete proc;
    return false;
  }
  proc->index = index;

  if (!readOptionalString(vm, reader, &proc->name))
  {
    vm->safetimeError("loadBytecode: failed to read process name at slot %u", slotIndex);
    delete proc;
    return false;
  }

  int32 totalFibersRaw = 0;
  if (!reader.readI32(&totalFibersRaw))
  {
    vm->safetimeError("loadBytecode: failed to read total fibers for process slot %u", slotIndex);
    delete proc;
    return false;
  }

  int32 nextFiberIndexRaw = 0;
  if (!reader.readI32(&nextFiberIndexRaw))
  {
    vm->safetimeError("loadBytecode: failed to read next fiber index for process slot %u", slotIndex);
    delete proc;
    return false;
  }

  uint32 argsCount = 0;
  if (!reader.readU32(&argsCount))
  {
    vm->safetimeError("loadBytecode: failed to read args count for process slot %u", slotIndex);
    delete proc;
    return false;
  }

  if (argsCount > 0)
  {
    std::vector<uint8> args(argsCount);
    if (!reader.readRaw(&args[0], (size_t)argsCount))
    {
      vm->safetimeError("loadBytecode: failed to read args mapping for process slot %u", slotIndex);
      delete proc;
      return false;
    }
    proc->argsNames.reserve((size_t)argsCount);
    for (uint32 i = 0; i < argsCount; ++i)
    {
      proc->argsNames.push(args[(size_t)i]);
    }
  }

  uint32 privatesCount = 0;
  if (!reader.readU32(&privatesCount))
  {
    vm->safetimeError("loadBytecode: failed to read privates count for process slot %u", slotIndex);
    delete proc;
    return false;
  }

  if (privatesCount != (uint32)MAX_PRIVATES)
  {
    vm->safetimeError("loadBytecode: invalid privates count for process slot %u (%u)", slotIndex, privatesCount);
    delete proc;
    return false;
  }

  for (uint32 i = 0; i < privatesCount; ++i)
  {
    if (!readValue(vm, reader, &proc->privates[i], "process private"))
    {
      delete proc;
      return false;
    }
  }

  uint32 fiberCount = 0;
  if (!reader.readU32(&fiberCount))
  {
    vm->safetimeError("loadBytecode: failed to read fiber count for process slot %u", slotIndex);
    delete proc;
    return false;
  }

  if (totalFibersRaw >= 0 && (uint32)totalFibersRaw != fiberCount)
  {
    vm->safetimeError("loadBytecode: inconsistent fiber count for process slot %u (%d vs %u)",
                      slotIndex, totalFibersRaw, fiberCount);
    delete proc;
    return false;
  }

  if (fiberCount != 1)
  {
    vm->safetimeError("loadBytecode: process slot %u has unsupported fiber count (%u), expected 1",
                      slotIndex, fiberCount);
    delete proc;
    return false;
  }

  if (nextFiberIndexRaw < 0 || nextFiberIndexRaw > 1)
  {
    vm->safetimeError("loadBytecode: invalid next fiber index for process slot %u (%d)",
                      slotIndex, nextFiberIndexRaw);
    delete proc;
    return false;
  }

  ProcessExec &fiber = *proc;
  fiber.state = ProcessState::DEAD;
  fiber.resumeTime = 0.0f;
  fiber.ip = nullptr;
  fiber.stackTop = fiber.stack;
  fiber.frameCount = 0;
  fiber.gosubTop = 0;
  fiber.tryDepth = 0;

  const int i = 0;
  uint8 rawState = 0;
  if (!reader.readU8(&rawState))
  {
    vm->safetimeError("loadBytecode: failed to read fiber state for process slot %u fiber %d", slotIndex, i);
    delete proc;
    return false;
  }

  if (rawState > (uint8)ProcessState::DEAD)
  {
    vm->safetimeError("loadBytecode: invalid fiber state %u for process slot %u fiber %d",
                      (unsigned)rawState, slotIndex, i);
    delete proc;
    return false;
  }
  fiber.state = (ProcessState)rawState;

  if (!reader.readF32(&fiber.resumeTime))
  {
    vm->safetimeError("loadBytecode: failed to read fiber resume time for process slot %u fiber %d",
                      slotIndex, i);
    delete proc;
    return false;
  }

  int32 frameCount = 0;
  int32 gosubTop = 0;
  int32 tryDepth = 0;
  uint32 serializedFrameCount = 0;

  if (!reader.readI32(&frameCount) ||
      !reader.readI32(&gosubTop) ||
      !reader.readI32(&tryDepth) ||
      !reader.readU32(&serializedFrameCount))
  {
    vm->safetimeError("loadBytecode: failed to read fiber frame metadata for process slot %u fiber %d",
                      slotIndex, i);
    delete proc;
    return false;
  }

  if (frameCount < 0 || frameCount > FRAMES_MAX || (uint32)frameCount != serializedFrameCount)
  {
    vm->safetimeError("loadBytecode: invalid frame count for process slot %u fiber %d (%d / %u)",
                      slotIndex, i, frameCount, serializedFrameCount);
    delete proc;
    return false;
  }
  fiber.frameCount = frameCount;

  fiber.gosubTop = std::max(0, std::min((int)GOSUB_MAX, (int)gosubTop));
  fiber.tryDepth = std::max(0, std::min((int)TRY_MAX, (int)tryDepth));

  for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
  {
    CallFrame &frame = fiber.frames[frameIndex];
    frame.func = nullptr;
    frame.ip = nullptr;
    frame.slots = fiber.stack;
    frame.closure = nullptr;

    int32 functionIndex = 0;
    uint32 ipOffset = 0;
    int32 slotOffset = 0;
    if (!reader.readI32(&functionIndex) ||
        !reader.readU32(&ipOffset) ||
        !reader.readI32(&slotOffset))
    {
      vm->safetimeError("loadBytecode: failed to read frame %d for process slot %u fiber %d",
                        frameIndex, slotIndex, i);
      delete proc;
      return false;
    }

    if (functionIndex >= 0)
    {
      if (!resolveFunctionByIndex(vm, functions, functionIndex, "process frame", &frame.func))
      {
        delete proc;
        return false;
      }
    }
    else if (ipOffset != kInvalidIpOffset)
    {
      vm->safetimeError("loadBytecode: frame %d in process slot %u fiber %d has ip offset without function",
                        frameIndex, slotIndex, i);
      delete proc;
      return false;
    }

    if (frame.func && ipOffset != kInvalidIpOffset)
    {
      if (ipOffset > frame.func->chunk->count)
      {
        vm->safetimeError("loadBytecode: invalid ip offset %u for frame %d in process slot %u fiber %d",
                          ipOffset, frameIndex, slotIndex, i);
        delete proc;
        return false;
      }
      frame.ip = frame.func->chunk->code + ipOffset;
    }

    if (slotOffset == -1)
    {
      frame.slots = fiber.stack;
    }
    else if (slotOffset >= 0 && slotOffset <= STACK_MAX)
    {
      frame.slots = fiber.stack + slotOffset;
    }
    else
    {
      vm->safetimeError("loadBytecode: invalid slot offset %d for frame %d in process slot %u fiber %d",
                        slotOffset, frameIndex, slotIndex, i);
      delete proc;
      return false;
    }
  }

  uint32 fiberIpOffset = 0;
  int32 stackSize = 0;
  if (!reader.readU32(&fiberIpOffset) || !reader.readI32(&stackSize))
  {
    vm->safetimeError("loadBytecode: failed to read fiber tail metadata for process slot %u fiber %d",
                      slotIndex, i);
    delete proc;
    return false;
  }

  if (stackSize == -1)
  {
    fiber.stackTop = fiber.stack;
  }
  else if (stackSize >= 0 && stackSize <= STACK_MAX)
  {
    fiber.stackTop = fiber.stack + stackSize;
  }
  else
  {
    vm->safetimeError("loadBytecode: invalid stack size %d for process slot %u fiber %d",
                      stackSize, slotIndex, i);
    delete proc;
    return false;
  }

  Function *baseFunc = frameCount > 0 ? fiber.frames[0].func : nullptr;
  if (baseFunc && fiberIpOffset != kInvalidIpOffset)
  {
    if (fiberIpOffset > baseFunc->chunk->count)
    {
      vm->safetimeError("loadBytecode: invalid fiber ip offset %u for process slot %u fiber %d",
                        fiberIpOffset, slotIndex, i);
      delete proc;
      return false;
    }
    fiber.ip = baseFunc->chunk->code + fiberIpOffset;
  }
  else
  {
    fiber.ip = nullptr;
  }

  proc->finalize();
  processes.push(proc);
  if (proc->name)
  {
    processesMap.set(proc->name, proc);
  }
  return true;
}

bool readStructRecord(Interpreter *vm,
                      BytecodeReader &reader,
                      uint32 slotIndex,
                      Vector<StructDef *> &structs,
                      HashMap<String *, StructDef *, StringHasher, StringEq> &structsMap)
{
  uint8 present = 0;
  if (!reader.readU8(&present))
  {
    vm->safetimeError("loadBytecode: failed to read struct presence at slot %u", slotIndex);
    return false;
  }

  if (present == 0)
  {
    structs.push(nullptr);
    return true;
  }

  if (present != 1)
  {
    vm->safetimeError("loadBytecode: invalid struct presence flag at slot %u", slotIndex);
    return false;
  }

  StructDef *def = new StructDef();
  def->index = -1;
  def->name = nullptr;
  def->argCount = 0;

  int32 index = 0;
  if (!reader.readI32(&index))
  {
    vm->safetimeError("loadBytecode: failed to read struct index at slot %u", slotIndex);
    delete def;
    return false;
  }
  if (index < 0 || (uint32)index != slotIndex)
  {
    vm->safetimeError("loadBytecode: struct index mismatch at slot %u (got %d)", slotIndex, index);
    delete def;
    return false;
  }
  def->index = index;

  if (!readOptionalString(vm, reader, &def->name))
  {
    vm->safetimeError("loadBytecode: failed to read struct name at slot %u", slotIndex);
    delete def;
    return false;
  }

  uint8 argCount = 0;
  if (!reader.readU8(&argCount))
  {
    vm->safetimeError("loadBytecode: failed to read struct arg count at slot %u", slotIndex);
    delete def;
    return false;
  }
  def->argCount = argCount;

  uint32 fieldsCount = 0;
  if (!reader.readU32(&fieldsCount))
  {
    vm->safetimeError("loadBytecode: failed to read struct fields count at slot %u", slotIndex);
    delete def;
    return false;
  }

  for (uint32 i = 0; i < fieldsCount; ++i)
  {
    String *fieldName = nullptr;
    if (!readString(vm, reader, &fieldName))
    {
      vm->safetimeError("loadBytecode: failed to read struct field name at slot %u", slotIndex);
      delete def;
      return false;
    }
    if (!fieldName)
    {
      vm->safetimeError("loadBytecode: null struct field name at slot %u", slotIndex);
      delete def;
      return false;
    }

    uint8 fieldIndex = 0;
    if (!reader.readU8(&fieldIndex))
    {
      vm->safetimeError("loadBytecode: failed to read struct field index at slot %u", slotIndex);
      delete def;
      return false;
    }

    def->names.set(fieldName, fieldIndex);
  }

  structs.push(def);
  if (def->name)
  {
    structsMap.set(def->name, def);
  }
  return true;
}

struct PendingClassLinks
{
  ClassDef *klass;
  int32 constructorIndex;
  int32 superclassIndex;
  int32 nativeSuperclassIndex;
  std::vector<std::pair<String *, int32> > methods;
};

bool readClassRecord(Interpreter *vm,
                     BytecodeReader &reader,
                     uint32 slotIndex,
                     Vector<ClassDef *> &classes,
                     HashMap<String *, ClassDef *, StringHasher, StringEq> &classesMap,
                     std::vector<PendingClassLinks> &pending)
{
  uint8 present = 0;
  if (!reader.readU8(&present))
  {
    vm->safetimeError("loadBytecode: failed to read class presence at slot %u", slotIndex);
    return false;
  }

  if (present == 0)
  {
    classes.push(nullptr);
    return true;
  }

  if (present != 1)
  {
    vm->safetimeError("loadBytecode: invalid class presence flag at slot %u", slotIndex);
    return false;
  }

  ClassDef *klass = new ClassDef();
  klass->index = -1;
  klass->name = nullptr;
  klass->parent = nullptr;
  klass->inherited = false;
  klass->fieldCount = 0;
  klass->constructor = nullptr;
  klass->superclass = nullptr;
  klass->nativeSuperclass = nullptr;

  int32 index = 0;
  if (!reader.readI32(&index))
  {
    vm->safetimeError("loadBytecode: failed to read class index at slot %u", slotIndex);
    delete klass;
    return false;
  }
  if (index < 0 || (uint32)index != slotIndex)
  {
    vm->safetimeError("loadBytecode: class index mismatch at slot %u (got %d)", slotIndex, index);
    delete klass;
    return false;
  }
  klass->index = index;

  if (!readOptionalString(vm, reader, &klass->name) ||
      !readOptionalString(vm, reader, &klass->parent))
  {
    vm->safetimeError("loadBytecode: failed to read class names at slot %u", slotIndex);
    delete klass;
    return false;
  }

  uint8 inherited = 0;
  if (!reader.readU8(&inherited))
  {
    vm->safetimeError("loadBytecode: failed to read class inherited flag at slot %u", slotIndex);
    delete klass;
    return false;
  }
  klass->inherited = inherited != 0;

  int32 fieldCount = 0;
  int32 constructorIndex = 0;
  int32 superclassIndex = 0;
  int32 nativeSuperclassIndex = 0;
  if (!reader.readI32(&fieldCount) ||
      !reader.readI32(&constructorIndex) ||
      !reader.readI32(&superclassIndex) ||
      !reader.readI32(&nativeSuperclassIndex))
  {
    vm->safetimeError("loadBytecode: failed to read class metadata at slot %u", slotIndex);
    delete klass;
    return false;
  }
  klass->fieldCount = fieldCount;

  uint32 fieldsCount = 0;
  if (!reader.readU32(&fieldsCount))
  {
    vm->safetimeError("loadBytecode: failed to read class field names count at slot %u", slotIndex);
    delete klass;
    return false;
  }

  for (uint32 i = 0; i < fieldsCount; ++i)
  {
    String *fieldName = nullptr;
    if (!readString(vm, reader, &fieldName))
    {
      vm->safetimeError("loadBytecode: failed to read class field name at slot %u", slotIndex);
      delete klass;
      return false;
    }
    if (!fieldName)
    {
      vm->safetimeError("loadBytecode: null class field name at slot %u", slotIndex);
      delete klass;
      return false;
    }

    uint8 fieldIndex = 0;
    if (!reader.readU8(&fieldIndex))
    {
      vm->safetimeError("loadBytecode: failed to read class field index at slot %u", slotIndex);
      delete klass;
      return false;
    }

    klass->fieldNames.set(fieldName, fieldIndex);
  }

  uint32 defaultsCount = 0;
  if (!reader.readU32(&defaultsCount))
  {
    vm->safetimeError("loadBytecode: failed to read class defaults count at slot %u", slotIndex);
    delete klass;
    return false;
  }

  klass->fieldDefaults.reserve((size_t)defaultsCount);
  for (uint32 i = 0; i < defaultsCount; ++i)
  {
    Value value;
    if (!readValue(vm, reader, &value, "class field default"))
    {
      delete klass;
      return false;
    }
    klass->fieldDefaults.push(value);
  }

  uint32 methodsCount = 0;
  if (!reader.readU32(&methodsCount))
  {
    vm->safetimeError("loadBytecode: failed to read class methods count at slot %u", slotIndex);
    delete klass;
    return false;
  }

  PendingClassLinks links;
  links.klass = klass;
  links.constructorIndex = constructorIndex;
  links.superclassIndex = superclassIndex;
  links.nativeSuperclassIndex = nativeSuperclassIndex;
  links.methods.reserve((size_t)methodsCount);

  for (uint32 i = 0; i < methodsCount; ++i)
  {
    String *methodName = nullptr;
    if (!readString(vm, reader, &methodName))
    {
      vm->safetimeError("loadBytecode: failed to read class method name at slot %u", slotIndex);
      delete klass;
      return false;
    }
    if (!methodName)
    {
      vm->safetimeError("loadBytecode: null class method name at slot %u", slotIndex);
      delete klass;
      return false;
    }

    int32 methodIndex = 0;
    if (!reader.readI32(&methodIndex))
    {
      vm->safetimeError("loadBytecode: failed to read class method ref at slot %u", slotIndex);
      delete klass;
      return false;
    }
    links.methods.push_back(std::make_pair(methodName, methodIndex));
  }

  classes.push(klass);
  if (klass->name)
  {
    classesMap.set(klass->name, klass);
  }
  pending.push_back(links);
  return true;
}

bool resolveClassLinks(Interpreter *vm,
                       const Vector<Function *> &functions,
                       const Vector<ClassDef *> &classes,
                       const Vector<NativeClassDef *> &nativeClasses,
                       const std::vector<PendingClassLinks> &pending)
{
  for (size_t i = 0; i < pending.size(); ++i)
  {
    const PendingClassLinks &links = pending[i];
    ClassDef *klass = links.klass;
    const char *className = klass->name ? klass->name->chars() : "<anonymous class>";

    if (links.constructorIndex >= 0)
    {
      if ((size_t)links.constructorIndex >= functions.size() || !functions[(size_t)links.constructorIndex])
      {
        vm->safetimeError("loadBytecode: invalid constructor index %d in class '%s'",
                          links.constructorIndex, className);
        return false;
      }
      klass->constructor = functions[(size_t)links.constructorIndex];
    }

    if (links.superclassIndex >= 0)
    {
      if ((size_t)links.superclassIndex >= classes.size() || !classes[(size_t)links.superclassIndex])
      {
        vm->safetimeError("loadBytecode: invalid superclass index %d in class '%s'",
                          links.superclassIndex, className);
        return false;
      }
      klass->superclass = classes[(size_t)links.superclassIndex];
    }

    if (links.nativeSuperclassIndex >= 0)
    {
      if ((size_t)links.nativeSuperclassIndex >= nativeClasses.size() ||
          !nativeClasses[(size_t)links.nativeSuperclassIndex])
      {
        vm->safetimeError("loadBytecode: invalid native superclass index %d in class '%s'",
                          links.nativeSuperclassIndex, className);
        return false;
      }
      klass->nativeSuperclass = nativeClasses[(size_t)links.nativeSuperclassIndex];
    }

    for (size_t j = 0; j < links.methods.size(); ++j)
    {
      String *methodName = links.methods[j].first;
      int32 methodIndex = links.methods[j].second;

      if (methodIndex < 0)
      {
        vm->safetimeError("loadBytecode: class method '%s' in class '%s' has invalid function reference (%d)",
                          methodName ? methodName->chars() : "<anonymous method>",
                          className, methodIndex);
        return false;
      }

      if ((size_t)methodIndex >= functions.size() || !functions[(size_t)methodIndex])
      {
        vm->safetimeError("loadBytecode: class method '%s' in class '%s' references unknown function %d",
                          methodName ? methodName->chars() : "<anonymous method>",
                          className, methodIndex);
        return false;
      }

      klass->methods.set(methodName, functions[(size_t)methodIndex]);
    }
  }

  return true;
}

bool readGlobalNames(Interpreter *vm,
                     BytecodeReader &reader,
                     uint32 globalsCount,
                     Vector<Value> &globalsArray,
                     Vector<String *> &globalIndexToName,
                     HashMap<String *, uint16, StringHasher, StringEq> &nativeGlobalIndices)
{
  globalIndexToName.clear();
  globalIndexToName.reserve((size_t)globalsCount);

  for (uint32 i = 0; i < globalsCount; ++i)
  {
    String *name = nullptr;
    if (!readOptionalString(vm, reader, &name))
    {
      vm->safetimeError("loadBytecode: failed to read global name at index %u", i);
      return false;
    }
    globalIndexToName.push(name);
  }

  while (globalsArray.size() < (size_t)globalsCount)
  {
    globalsArray.push(vm->makeNil());
  }

  for (uint32 i = 0; i < globalsCount; ++i)
  {
    String *name = globalIndexToName[(size_t)i];
    uint16 nativeIndex = 0;
    bool isNativeSlot = name && nativeGlobalIndices.get(name, &nativeIndex);

    if (isNativeSlot)
    {
      if (nativeIndex != i)
      {
        vm->safetimeError("loadBytecode: native global '%s' index mismatch (file=%u runtime=%u)",
                          name->chars(), i, nativeIndex);
        return false;
      }

      if ((size_t)i >= globalsArray.size())
      {
        vm->safetimeError("loadBytecode: global '%s' is missing at index %u", name->chars(), i);
        return false;
      }
      continue;
    }

    globalsArray[(size_t)i] = vm->makeNil();
  }

  return true;
}

bool validateNatives(Interpreter *vm, BytecodeReader &reader, uint32 nativesCount, const Vector<NativeDef> &natives)
{
  if (nativesCount > (uint32)natives.size())
  {
    vm->safetimeError("loadBytecode: file requires %u native functions, runtime has %zu",
                      nativesCount, natives.size());
    return false;
  }

  for (uint32 i = 0; i < nativesCount; ++i)
  {
    int32 index = 0;
    String *name = nullptr;
    int32 arity = 0;
    if (!reader.readI32(&index) ||
        !readOptionalString(vm, reader, &name) ||
        !reader.readI32(&arity))
    {
      vm->safetimeError("loadBytecode: failed to read native metadata at slot %u", i);
      return false;
    }

    if (index < 0 || (size_t)index >= natives.size())
    {
      vm->safetimeError("loadBytecode: invalid native index %d in file", index);
      return false;
    }

    const NativeDef &runtime = natives[(size_t)index];
    if (name && !stringEquals(name, runtime.name))
    {
      vm->safetimeError("loadBytecode: native mismatch at index %d (file='%s' runtime='%s')",
                        index,
                        name->chars(),
                        runtime.name ? runtime.name->chars() : "<null>");
      return false;
    }

    if (arity != runtime.arity)
    {
      vm->safetimeError("loadBytecode: native arity mismatch at index %d (file=%d runtime=%d)",
                        index, arity, runtime.arity);
      return false;
    }
  }

  return true;
}

bool validateNativeProcesses(Interpreter *vm,
                             BytecodeReader &reader,
                             uint32 nativeProcessesCount,
                             const Vector<NativeProcessDef> &nativeProcesses)
{
  if (nativeProcessesCount > (uint32)nativeProcesses.size())
  {
    vm->safetimeError("loadBytecode: file requires %u native processes, runtime has %zu",
                      nativeProcessesCount, nativeProcesses.size());
    return false;
  }

  for (uint32 i = 0; i < nativeProcessesCount; ++i)
  {
    int32 index = 0;
    String *name = nullptr;
    int32 arity = 0;
    if (!reader.readI32(&index) ||
        !readOptionalString(vm, reader, &name) ||
        !reader.readI32(&arity))
    {
      vm->safetimeError("loadBytecode: failed to read native process metadata at slot %u", i);
      return false;
    }

    if (index < 0 || (size_t)index >= nativeProcesses.size())
    {
      vm->safetimeError("loadBytecode: invalid native process index %d in file", index);
      return false;
    }

    const NativeProcessDef &runtime = nativeProcesses[(size_t)index];
    if (name && !stringEquals(name, runtime.name))
    {
      vm->safetimeError("loadBytecode: native process mismatch at index %d (file='%s' runtime='%s')",
                        index,
                        name->chars(),
                        runtime.name ? runtime.name->chars() : "<null>");
      return false;
    }

    if (arity != runtime.arity)
    {
      vm->safetimeError("loadBytecode: native process arity mismatch at index %d (file=%d runtime=%d)",
                        index, arity, runtime.arity);
      return false;
    }
  }

  return true;
}

bool validateModules(Interpreter *vm, BytecodeReader &reader, uint32 modulesCount, const Vector<ModuleDef *> &modules)
{
  if (modulesCount > (uint32)modules.size())
  {
    vm->safetimeError("loadBytecode: file requires %u modules, runtime has %zu", modulesCount, modules.size());
    return false;
  }

  for (uint32 i = 0; i < modulesCount; ++i)
  {
    uint8 present = 0;
    if (!reader.readU8(&present))
    {
      vm->safetimeError("loadBytecode: failed to read module presence at slot %u", i);
      return false;
    }

    ModuleDef *runtimeModule = modules[(size_t)i];
    if (present == 0)
    {
      if (runtimeModule)
      {
        vm->safetimeError("loadBytecode: module slot %u is null in file but present in runtime", i);
        return false;
      }
      continue;
    }

    if (present != 1 || !runtimeModule)
    {
      vm->safetimeError("loadBytecode: invalid module presence at slot %u", i);
      return false;
    }

    String *fileName = nullptr;
    if (!readOptionalString(vm, reader, &fileName))
    {
      vm->safetimeError("loadBytecode: failed to read module name at slot %u", i);
      return false;
    }

    String *runtimeName = runtimeModule->getName();
    if (!stringEquals(fileName, runtimeName))
    {
      vm->safetimeError("loadBytecode: module name mismatch at slot %u (file='%s' runtime='%s')",
                        i,
                        fileName ? fileName->chars() : "<null>",
                        runtimeName ? runtimeName->chars() : "<null>");
      return false;
    }

    uint32 functionsCount = 0;
    if (!reader.readU32(&functionsCount))
    {
      vm->safetimeError("loadBytecode: failed to read module function count at slot %u", i);
      return false;
    }

    if (functionsCount > (uint32)runtimeModule->functions.size())
    {
      vm->safetimeError("loadBytecode: module '%s' function count mismatch (file=%u runtime=%zu)",
                        runtimeName ? runtimeName->chars() : "<unnamed>",
                        functionsCount,
                        runtimeModule->functions.size());
      return false;
    }

    for (uint32 j = 0; j < functionsCount; ++j)
    {
      String *fileFuncName = nullptr;
      int32 fileArity = 0;
      if (!readOptionalString(vm, reader, &fileFuncName) || !reader.readI32(&fileArity))
      {
        vm->safetimeError("loadBytecode: failed to read module function metadata at slot %u:%u", i, j);
        return false;
      }

      String *runtimeFuncName = nullptr;
      if (!runtimeModule->getFunctionName((uint16)j, &runtimeFuncName))
      {
        vm->safetimeError("loadBytecode: runtime module '%s' is missing function name for id %u",
                          runtimeName ? runtimeName->chars() : "<unnamed>", j);
        return false;
      }

      if (!stringEquals(fileFuncName, runtimeFuncName))
      {
        vm->safetimeError("loadBytecode: module function name mismatch at %s[%u] (file='%s' runtime='%s')",
                          runtimeName ? runtimeName->chars() : "<unnamed>",
                          j,
                          fileFuncName ? fileFuncName->chars() : "<null>",
                          runtimeFuncName ? runtimeFuncName->chars() : "<null>");
        return false;
      }

      if (fileArity != runtimeModule->functions[(size_t)j].arity)
      {
        vm->safetimeError("loadBytecode: module function arity mismatch at %s[%u] (file=%d runtime=%d)",
                          runtimeName ? runtimeName->chars() : "<unnamed>",
                          j,
                          fileArity,
                          runtimeModule->functions[(size_t)j].arity);
        return false;
      }
    }

    uint32 constantsCount = 0;
    if (!reader.readU32(&constantsCount))
    {
      vm->safetimeError("loadBytecode: failed to read module constants count at slot %u", i);
      return false;
    }
    if (constantsCount != 0u)
    {
      vm->safetimeError("loadBytecode: module constants are not supported in format v1 (module slot %u)", i);
      return false;
    }
  }

  return true;
}

} // namespace

bool Interpreter::loadBytecode(const char *filename)
{
  if (!filename || filename[0] == '\0')
  {
    safetimeError("loadBytecode: invalid input path");
    return false;
  }

  const uint8 *bytecodeData = nullptr;
  size_t bytecodeSize = 0;

  if (fileLoaderCallback_)
  {
    size_t loadedSize = 0;
    const char *loadedData = fileLoaderCallback_(filename, &loadedSize, fileLoaderUserdata_);
    if (loadedData && loadedSize > 0)
    {
      bytecodeData = reinterpret_cast<const uint8 *>(loadedData);
      bytecodeSize = loadedSize;
    }
  }

  FILE *file = nullptr;
  if (!bytecodeData)
  {
    file = std::fopen(filename, "rb");
    if (!file)
    {
      safetimeError("loadBytecode: failed to open '%s' for reading", filename);
      return false;
    }
  }

  auto closeInputFile = [&]()
  {
    if (file)
    {
      std::fclose(file);
      file = nullptr;
    }
  };

  BytecodeReader reader = bytecodeData
                              ? BytecodeReader(bytecodeData, bytecodeSize)
                              : BytecodeReader(file);

  uint8 magic[sizeof(BytecodeFormat::MAGIC)] = {};
  uint16 versionMajor = 0;
  uint16 versionMinor = 0;
  uint32 sectionFlags = 0u;
  uint32 functionsCount = 0u;
  uint32 processesCount = 0u;
  uint32 structsCount = 0u;
  uint32 classesCount = 0u;
  uint32 globalsCount = 0u;
  uint32 nativesCount = 0u;
  uint32 nativeProcessesCount = 0u;
  uint32 modulesCount = 0u;

  bool ok = reader.readRaw(magic, sizeof(magic)) &&
            reader.readU16(&versionMajor) &&
            reader.readU16(&versionMinor) &&
            reader.readU32(&sectionFlags) &&
            reader.readU32(&functionsCount) &&
            reader.readU32(&processesCount) &&
            reader.readU32(&structsCount) &&
            reader.readU32(&classesCount) &&
            reader.readU32(&globalsCount) &&
            reader.readU32(&nativesCount) &&
            reader.readU32(&nativeProcessesCount) &&
            reader.readU32(&modulesCount);

  if (!ok)
  {
    safetimeError("loadBytecode: failed to read header from '%s'", filename);
    closeInputFile();
    return false;
  }

  if (std::memcmp(magic, BytecodeFormat::MAGIC, sizeof(BytecodeFormat::MAGIC)) != 0)
  {
    safetimeError("loadBytecode: invalid magic in '%s'", filename);
    closeInputFile();
    return false;
  }

  if (versionMajor != BytecodeFormat::VERSION_MAJOR ||
      versionMinor != BytecodeFormat::VERSION_MINOR)
  {
    safetimeError("loadBytecode: unsupported bytecode version %u.%u in '%s' (expected %u.%u)",
                  versionMajor,
                  versionMinor,
                  filename,
                  BytecodeFormat::VERSION_MAJOR,
                  BytecodeFormat::VERSION_MINOR);
    closeInputFile();
    return false;
  }

  (void)sectionFlags;

  reset();

  std::vector<PendingClassLinks> pendingClassLinks;
  pendingClassLinks.reserve((size_t)classesCount);

  for (uint32 i = 0; ok && i < functionsCount; ++i)
  {
    ok = readFunctionRecord(this, reader, i, functions, functionsMap);
  }

  for (uint32 i = 0; ok && i < processesCount; ++i)
  {
    ok = readProcessRecord(this, reader, i, functions, processes, processesMap);
  }

  for (uint32 i = 0; ok && i < structsCount; ++i)
  {
    ok = readStructRecord(this, reader, i, structs, structsMap);
  }

  for (uint32 i = 0; ok && i < classesCount; ++i)
  {
    ok = readClassRecord(this, reader, i, classes, classesMap, pendingClassLinks);
  }

  if (ok)
  {
    ok = resolveClassLinks(this, functions, classes, nativeClasses, pendingClassLinks);
  }

  if (ok)
  {
    ok = readGlobalNames(this, reader, globalsCount, globalsArray, globalIndexToName_, nativeGlobalIndices);
  }

  if (ok)
  {
    ok = validateNatives(this, reader, nativesCount, natives);
  }

  if (ok)
  {
    ok = validateNativeProcesses(this, reader, nativeProcessesCount, nativeProcesses);
  }

  if (ok)
  {
    ok = validateModules(this, reader, modulesCount, modules);
  }

  if (!ok || !reader.ok())
  {
    safetimeError("loadBytecode: failed to deserialize '%s'", filename);
    closeInputFile();
    reset();
    return false;
  }

  closeInputFile();

  // Match Interpreter::run() bootstrap behavior:
  // spawn the first process as main and execute initial script pass.
  if (!processes.empty() && processes[0] != nullptr)
  {
    mainProcess = spawnProcess(processes[0]);
    if (!mainProcess)
    {
      safetimeError("loadBytecode: failed to spawn main process from '%s'", filename);
      reset();
      return false;
    }

    currentProcess = mainProcess;
    run_process(mainProcess);
    if (hasFatalError_)
    {
      safetimeError("loadBytecode: fatal error while bootstrapping '%s'", filename);
      return false;
    }
  }

  return true;
}
