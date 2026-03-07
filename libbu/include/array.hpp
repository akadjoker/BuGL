
#pragma once
#include "value.hpp"

struct Array
{
  Value *data = nullptr;
  size_t count = 0;
  size_t capacity = 0;

  Array()
  {
  }

  ~Array() { destroy(); }

  Array(const Array &) = delete;
  Array &operator=(const Array &) = delete;

  void destroy();

  void reserve(size_t newCap);

  void push(Value v);

  Value pop();

  Value back();
  const Value &back() const;

  Value &operator[](size_t i);
  const Value &operator[](size_t i) const;

  size_t size() const;
};