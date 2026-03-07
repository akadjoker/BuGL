#include "debug.hpp"
#include "code.hpp"
#include "interpreter.hpp"
#include "opcode.hpp"
#include <cstdio>

// Global names para disassembly
static const char** g_globalNames = nullptr;
static int g_globalNamesCount = 0;

void Debug::setGlobalNames(const char** names, int count)
{
  g_globalNames = names;
  g_globalNamesCount = count;
}

void Debug::clearGlobalNames()
{
  g_globalNames = nullptr;
  g_globalNamesCount = 0;
}

void Debug::disassembleChunk(const Code &chunk, const char *name)
{
  printf("== %s ==\n", name);

  for (size_t offset = 0; offset < chunk.count;)
  {
    offset = disassembleInstruction(chunk, offset);
  }
}

static bool hasBytes(const Code &chunk, size_t offset, size_t n)
{
  return offset + n < chunk.count;
}

size_t Debug::disassembleInstruction(const Code &chunk, size_t offset)
{
  printf("%04zu ", offset);

  if (offset > 0 && chunk.lines[offset] == chunk.lines[offset - 1])
    printf("   | ");
  else
    printf("%4d ", chunk.lines[offset]);

  if (offset >= chunk.count)
  {
    printf("<<out of bounds>>\n");
    return offset + 1;
  }

  uint8 instruction = chunk.code[offset];

  switch (instruction)
  {
    // ========== LITERALS (0-3) ==========
  case OP_CONSTANT:
  {
    if (!hasBytes(chunk, offset, 2))
    {
      printf("OP_CONSTANT <truncated>\n");
      return chunk.count;
    }
    uint16_t constant = (uint16_t)(chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
    printf("%-20s %4d '", "OP_CONSTANT", constant);
    printValue(chunk.constants[constant]);
    printf("'\n");
    return offset + 3;
  }
  case OP_NIL:
    return simpleInstruction("OP_NIL", offset);
  case OP_TRUE:
    return simpleInstruction("OP_TRUE", offset);
  case OP_FALSE:
    return simpleInstruction("OP_FALSE", offset);

    // ========== STACK (4-7) ==========
  case OP_POP:
    return simpleInstruction("OP_POP", offset);
  case OP_HALT:
    return simpleInstruction("OP_HALT", offset);
  case OP_NOT:
    return simpleInstruction("OP_NOT", offset);
  case OP_DUP:
    return simpleInstruction("OP_DUP", offset);

    // ========== ARITHMETIC (8-13) ==========
  case OP_ADD:
    return simpleInstruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simpleInstruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simpleInstruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simpleInstruction("OP_DIVIDE", offset);
  case OP_NEGATE:
    return simpleInstruction("OP_NEGATE", offset);
  case OP_MODULO:
    return simpleInstruction("OP_MODULO", offset);

    // ========== BITWISE (14-19) ==========
  case OP_BITWISE_AND:
    return simpleInstruction("OP_BITWISE_AND", offset);
  case OP_BITWISE_OR:
    return simpleInstruction("OP_BITWISE_OR", offset);
  case OP_BITWISE_XOR:
    return simpleInstruction("OP_BITWISE_XOR", offset);
  case OP_BITWISE_NOT:
    return simpleInstruction("OP_BITWISE_NOT", offset);
  case OP_SHIFT_LEFT:
    return simpleInstruction("OP_SHIFT_LEFT", offset);
  case OP_SHIFT_RIGHT:
    return simpleInstruction("OP_SHIFT_RIGHT", offset);

    // ========== COMPARISONS (20-25) ==========
  case OP_EQUAL:
    return simpleInstruction("OP_EQUAL", offset);
  case OP_NOT_EQUAL:
    return simpleInstruction("OP_NOT_EQUAL", offset);
  case OP_GREATER:
    return simpleInstruction("OP_GREATER", offset);
  case OP_GREATER_EQUAL:
    return simpleInstruction("OP_GREATER_EQUAL", offset);
  case OP_LESS:
    return simpleInstruction("OP_LESS", offset);
  case OP_LESS_EQUAL:
    return simpleInstruction("OP_LESS_EQUAL", offset);

    // ========== VARIABLES (26-32) ==========
  case OP_GET_LOCAL:
    return byteInstruction("OP_GET_LOCAL", chunk, offset);
  case OP_SET_LOCAL:
    return byteInstruction("OP_SET_LOCAL", chunk, offset);
  case OP_GET_GLOBAL:
    return globalIndexInstruction("OP_GET_GLOBAL", chunk, offset);
  case OP_SET_GLOBAL:
    return globalIndexInstruction("OP_SET_GLOBAL", chunk, offset);
  case OP_DEFINE_GLOBAL:
    return globalIndexInstruction("OP_DEFINE_GLOBAL", chunk, offset);
  case OP_GET_PRIVATE:
    return byteInstruction("OP_GET_PRIVATE", chunk, offset);
  case OP_SET_PRIVATE:
    return byteInstruction("OP_SET_PRIVATE", chunk, offset);

    // ========== CONTROL FLOW (33-37) ==========
  case OP_JUMP:
    return jumpInstruction("OP_JUMP", +1, chunk, offset);
  case OP_JUMP_IF_FALSE:
    return jumpInstruction("OP_JUMP_IF_FALSE", +1, chunk, offset);
  case OP_LOOP:
    return jumpInstruction("OP_LOOP", -1, chunk, offset);
  case OP_GOSUB:
    return jumpInstruction("OP_GOSUB", +1, chunk, offset);
  case OP_RETURN_SUB:
    return simpleInstruction("OP_RETURN_SUB", offset);

    // ========== FUNCTIONS (38-43) ==========
  case OP_CALL:
    return byteInstruction("OP_CALL", chunk, offset);
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  case OP_RETURN_N:
    return byteInstruction("OP_RETURN_N", chunk, offset);
  case OP_TYPE:
    return simpleInstruction("OP_TYPE", offset);
  case OP_PROC:
    return simpleInstruction("OP_PROC", offset);
  case OP_GET_ID:
    return simpleInstruction("OP_GET_ID", offset);

  case OP_CLOSURE:
  {
    if (!hasBytes(chunk, offset, 2))
    {
      printf("OP_CLOSURE <truncated>\n");
      return chunk.count;
    }

    offset++; // Avança para os bytes do constant index
    uint16 constant = (uint16)(chunk.code[offset] << 8) | chunk.code[offset + 1];
    offset += 2;
    printf("%-20s %4d '", "OP_CLOSURE", constant);
    printValue(chunk.constants[constant]);
    printf("'\n");

    // Lê upvalue info
    // Value funcVal = chunk.constants[constant];
    // if (funcVal.isFunction())
    // {
    //   Function *func = funcVal.asFunction();
    //   for (int i = 0; i < func->upvalueCount; i++)
    //   {
    //     if (!hasBytes(chunk, offset, 2))
    //     {
    //       printf("                     <truncated upvalue info>\n");
    //       return chunk.count;
    //     }

    //     bool isLocal = chunk.code[offset++];
    //     uint8 index = chunk.code[offset++];
    //     printf("%04zu      |                     %s %d\n",
    //            offset - 2, isLocal ? "local" : "upvalue", index);
    //   }
    // }

    return offset;
  }

  case OP_GET_UPVALUE:
    return byteInstruction("OP_GET_UPVALUE", chunk, offset);
  case OP_SET_UPVALUE:
    return byteInstruction("OP_SET_UPVALUE", chunk, offset);
  case OP_CLOSE_UPVALUE:
    return simpleInstruction("OP_CLOSE_UPVALUE", offset);
  case OP_ARRAY_PUSH:
    return byteInstruction("OP_ARRAY_PUSH", chunk, offset);
  case OP_RESERVED_41:
    return simpleInstruction("OP_RESERVED_41", offset);
  case OP_FRAME:
    return simpleInstruction("OP_FRAME", offset);
  case OP_EXIT:
    return simpleInstruction("OP_EXIT", offset);

    // ========== COLLECTIONS (44-45) ==========
  case OP_DEFINE_ARRAY:
    return shortInstruction("OP_DEFINE_ARRAY", chunk, offset);
  case OP_DEFINE_MAP:
    return shortInstruction("OP_DEFINE_MAP", chunk, offset);

    // ========== PROPERTIES (46-49) ==========
  case OP_GET_PROPERTY:
    return constantInstruction("OP_GET_PROPERTY", chunk, offset);
  case OP_SET_PROPERTY:
    return constantInstruction("OP_SET_PROPERTY", chunk, offset);
  case OP_GET_INDEX:
    return simpleInstruction("OP_GET_INDEX", offset);
  case OP_SET_INDEX:
    return simpleInstruction("OP_SET_INDEX", offset);

    // ========== METHODS (50-51) ==========
  case OP_INVOKE:
  {
    if (!hasBytes(chunk, offset, 3))
    {
      printf("OP_INVOKE <truncated>\n");
      return chunk.count;
    }

    uint16_t nameIdx = (uint16_t)(chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
    uint8_t argCount = chunk.code[offset + 3];

    Value c = chunk.constants[nameIdx];
    const char *nm = (c.isString() ? c.asString()->chars() : "<non-string>");

    printf("%-20s %4u '%s' (%u args)\n", "OP_INVOKE", (unsigned)nameIdx, nm,
           (unsigned)argCount);

    return offset + 4;
  }

  case OP_SUPER_INVOKE:
  {
    if (!hasBytes(chunk, offset, 4))
    {
      printf("OP_SUPER_INVOKE <truncated>\n");
      return chunk.count;
    }

    uint8_t ownerClassId = chunk.code[offset + 1];
    uint16_t nameIdx = (uint16_t)(chunk.code[offset + 2] << 8) | chunk.code[offset + 3];
    uint8_t argCount = chunk.code[offset + 4];

    Value c = chunk.constants[nameIdx];
    const char *nm = (c.isString() ? c.asString()->chars() : "<non-string>");

    printf("%-20s class=%u name=%u '%s' (%u args)\n", "OP_SUPER_INVOKE",
           (unsigned)ownerClassId, (unsigned)nameIdx, nm, (unsigned)argCount);

    return offset + 5;
  }

    // ========== I/O (52-53) ==========
  case OP_PRINT:
    return byteInstruction("OP_PRINT", chunk, offset);
  case OP_FUNC_LEN:
    return simpleInstruction("OP_FUNC_LEN", offset);

    // ========== FOREACH (54-58) ==========
  case OP_ITER_NEXT:
    return simpleInstruction("OP_ITER_NEXT", offset);
  case OP_ITER_VALUE:
    return simpleInstruction("OP_ITER_VALUE", offset);
  case OP_COPY2:
    return simpleInstruction("OP_COPY2", offset);
  case OP_SWAP:
    return simpleInstruction("OP_SWAP", offset);
  case OP_DISCARD:
    return byteInstruction("OP_DISCARD", chunk, offset);

    // ========== EXCEPTION HANDLING (59-64) ==========
  case OP_TRY:
  {
    if (!hasBytes(chunk, offset, 4))
    {
      printf("OP_TRY <truncated>\n");
      return chunk.count;
    }

    uint16_t catchAddr = (uint16_t)(chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
    uint16_t finallyAddr = (uint16_t)(chunk.code[offset + 3] << 8) | chunk.code[offset + 4];

    printf("%-20s catch=%04x finally=%04x\n", "OP_TRY",
           catchAddr, finallyAddr);

    return offset + 5;
  }
  case OP_POP_TRY:
    return simpleInstruction("OP_POP_TRY", offset);
  case OP_THROW:
    return simpleInstruction("OP_THROW", offset);
  case OP_ENTER_CATCH:
    return simpleInstruction("OP_ENTER_CATCH", offset);
  case OP_ENTER_FINALLY:
    return simpleInstruction("OP_ENTER_FINALLY", offset);
  case OP_EXIT_FINALLY:
    return simpleInstruction("OP_EXIT_FINALLY", offset);

    // ========== MATH UNARY (65-78) ==========
  case OP_SIN:
    return simpleInstruction("OP_SIN", offset);
  case OP_COS:
    return simpleInstruction("OP_COS", offset);
  case OP_TAN:
    return simpleInstruction("OP_TAN", offset);
  case OP_ASIN:
    return simpleInstruction("OP_ASIN", offset);
  case OP_ACOS:
    return simpleInstruction("OP_ACOS", offset);
  case OP_ATAN:
    return simpleInstruction("OP_ATAN", offset);
  case OP_SQRT:
    return simpleInstruction("OP_SQRT", offset);
  case OP_ABS:
    return simpleInstruction("OP_ABS", offset);
  case OP_LOG:
    return simpleInstruction("OP_LOG", offset);
  case OP_FLOOR:
    return simpleInstruction("OP_FLOOR", offset);
  case OP_CEIL:
    return simpleInstruction("OP_CEIL", offset);
  case OP_DEG:
    return simpleInstruction("OP_DEG", offset);
  case OP_RAD:
    return simpleInstruction("OP_RAD", offset);
  case OP_EXP:
    return simpleInstruction("OP_EXP", offset);

    // ========== MATH BINARY (79-80) ==========
  case OP_ATAN2:
    return simpleInstruction("OP_ATAN2", offset);
  case OP_POW:
    return simpleInstruction("OP_POW", offset);

    // ========== UTILITIES (81-83) ==========
  case OP_CLOCK:
    return simpleInstruction("OP_CLOCK", offset);
  case OP_NEW_BUFFER:
    return simpleInstruction("OP_NEW_BUFFER", offset);
  case OP_FREE:
    return simpleInstruction("OP_FREE", offset);

  default:
    printf("Unknown opcode %u\n", (unsigned)instruction);
    return offset + 1;
  }
}

size_t Debug::simpleInstruction(const char *name, size_t offset)
{
  printf("%-20s\n", name);
  return offset + 1;
}

size_t Debug::constantInstruction(const char *name, const Code &chunk,
                                  size_t offset)
{
  if (!hasBytes(chunk, offset, 2))
  {
    printf("%s <truncated>\n", name);
    return chunk.count;
  }

  uint16 constantIdx = (uint16)(chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
  printf("%-20s %4u '", name, (unsigned)constantIdx);
  printValue(chunk.constants[constantIdx]);
  printf("'\n");
  return offset + 3;
}

size_t Debug::constantNameInstruction(const char *name, const Code &chunk,
                                      size_t offset)
{
  if (!hasBytes(chunk, offset, 2))
  {
    printf("%s <truncated>\n", name);
    return chunk.count;
  }

  uint16 constantIdx = (uint16)(chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
  Value c = chunk.constants[constantIdx];
  const char *nm = (c.isString() ? c.asString()->chars() : "<non-string>");

  printf("%-20s %4u '%s'\n", name, (unsigned)constantIdx, nm);
  return offset + 3;
}

size_t Debug::globalIndexInstruction(const char *name, const Code &chunk,
                                     size_t offset)
{
  if (!hasBytes(chunk, offset, 2))
  {
    printf("%s <truncated>\n", name);
    return chunk.count;
  }

  uint16 globalIdx = (uint16)(chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
  
  // Se temos nomes globais, mostra o nome
  if (g_globalNames && globalIdx < g_globalNamesCount && g_globalNames[globalIdx])
  {
    printf("%-20s %4u '%s'\n", name, (unsigned)globalIdx, g_globalNames[globalIdx]);
  }
  else
  {
    printf("%-20s %4u\n", name, (unsigned)globalIdx);
  }
  
  return offset + 3;
}

size_t Debug::byteInstruction(const char *name, const Code &chunk,
                              size_t offset)
{
  if (!hasBytes(chunk, offset, 1))
  {
    printf("%s <truncated>\n", name);
    return chunk.count;
  }

  uint8 operand = chunk.code[offset + 1];
  printf("%-20s %4u\n", name, (unsigned)operand);
  return offset + 2;
}

size_t Debug::shortInstruction(const char *name, const Code &chunk,
                               size_t offset)
{
  if (!hasBytes(chunk, offset, 2))
  {
    printf("%s <truncated>\n", name);
    return chunk.count;
  }

  uint16 operand = (uint16)(chunk.code[offset + 1] << 8) | (uint16)chunk.code[offset + 2];
  printf("%-20s %4u\n", name, (unsigned)operand);
  return offset + 3;
}

size_t Debug::jumpInstruction(const char *name, int sign, const Code &chunk,
                              size_t offset)
{
  if (!hasBytes(chunk, offset, 2))
  {
    printf("%s <truncated>\n", name);
    return chunk.count;
  }

  uint16 jump =
      (uint16)(chunk.code[offset + 1] << 8) | (uint16)chunk.code[offset + 2];
  long long target = (long long)offset + 3 + (long long)sign * (long long)jump;

  printf("%-20s %4zu -> %lld\n", name, offset, target);
  return offset + 3;
}

void Debug::dumpFunction(const Function *func)
{
  const char *name = (func->name && func->name->length() > 0)
                         ? func->name->chars()
                         : "<script>";

  printf("\n========================================\n");
  printf("Function: %s\n", name);
  printf("Arity: %d\n", func->arity);
  printf("Has Return: %s\n", func->hasReturn ? "yes" : "no");
  printf("========================================\n\n");

  // ---- CONSTANTS ----
  if (func->chunk->constants.size() > 0)
  {
    printf("Constants (%zu):\n", func->chunk->constants.size());
    for (size_t i = 0; i < func->chunk->constants.size(); i++)
    {
      printf("  [%4zu] = ", i);
      printValue(func->chunk->constants[i]);
      printf("\n");
    }
    printf("\n");
  }

  // ---- BYTECODE ----
  disassembleChunk(*func->chunk, name);
  printf("\n");
}
