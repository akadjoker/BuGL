#pragma once

#include "config.hpp"

namespace BytecodeFormat
{
static constexpr uint8 MAGIC[4] = {'B', 'U', 'B', 'C'};
static constexpr uint16 VERSION_MAJOR = 1;
static constexpr uint16 VERSION_MINOR = 0;

enum SectionFlags : uint32
{
  HAS_PROCESSES = 1u << 0,
  HAS_STRUCTS = 1u << 1,
  HAS_CLASSES = 1u << 2,
  HAS_GLOBAL_NAMES = 1u << 3
};

enum class ConstantTag : uint8
{
  NIL = 0,
  BOOL = 1,
  BYTE = 2,
  INT = 3,
  UINT = 4,
  FLOAT = 5,
  DOUBLE = 6,
  STRING = 7,
  FUNCTION_REF = 8,
  PROCESS_REF = 9,
  STRUCT_REF = 10,
  CLASS_REF = 11,
  NATIVE_REF = 12,
  NATIVE_PROCESS_REF = 13,
  NATIVE_CLASS_REF = 14,
  NATIVE_STRUCT_REF = 15,
  MODULE_REF = 16
};
} // namespace BytecodeFormat
