#pragma once

#include "config.hpp"
#include "array.hpp"

struct Value;

struct ValueHasher
{
    size_t operator()(const Value &v) const;
  
};
struct ValueEq
{
    bool operator()(const Value& a, const Value& b) const;
    
};

class Code
{
    size_t m_capacity;
    bool m_frozen;
    int16 nilIndex,trueIndex,falseIndex;
     

public:
    Code(size_t capacity = 1024);

    void freeze();

    Code(const Code &other) = delete;
    Code &operator=(const Code &other) = delete;

    void clear();
    void reserve(size_t capacity);

    void write(uint8 instruction, int line);
    void writeShort(uint16 value, int line);

    size_t capacity() const { return m_capacity; }

    uint8 operator[](size_t index);

    int addConstant(Value value);

    uint8 *code;
    int *lines;
    size_t count;
    Array constants;
};