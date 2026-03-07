#pragma once
#include "arena.hpp"
#include "types.hpp"

struct String
{
  static constexpr size_t SMALL_THRESHOLD = 23;
  static constexpr size_t IS_LONG_FLAG = 0x80000000u;
  
  int index;
  size_t hash;
  size_t length_and_flag;

  union
  {
    char *ptr;
    char data[24];
  };

  FORCE_INLINE bool isLong() const { return length_and_flag & IS_LONG_FLAG; }
  FORCE_INLINE size_t length() const { return length_and_flag & ~IS_LONG_FLAG; };

  FORCE_INLINE const char *chars() const { return isLong() ? ptr : data; }
  FORCE_INLINE char *chars() { return isLong() ? ptr : data; }
};

inline size_t hashString(const char *s, uint32 len)
{
  size_t h = 2166136261u;
  const uint8 *p = (const uint8 *)s;
  const uint8 *end = p + len;
  
  while (p != end)
  {
    h ^= *p++;
    h *= 16777619u;
  }
  return h;
}

static inline bool compare_strings(String *a, String *b)
{
  if (a == b)
    return true;
  if (!a || !b)
    return false;
  if (a->hash != b->hash)
    return false;
  const size_t aLen = a->length();
  if (aLen != b->length())
    return false;
  return memcmp(a->chars(), b->chars(), aLen) == 0;
}
struct IntEq
{
  bool operator()(int a, int b) const { return a == b; }
};

struct StringEq
{
  bool operator()(String *a, String *b) const
  {
    return compare_strings(a, b);
  }
};

struct StringHasher
{
  size_t operator()(String *x) const { return x->hash; }
};


struct StringCmp 
{
    bool operator()(String* a, String* b) const 
    { 
       if (compare_strings(a, b)) 
          return false;
 
        
        // Se não são iguais, compara lexicograficamente
        size_t minLen = a->length() < b->length() ? a->length() : b->length();
        int cmp = memcmp(a->chars(), b->chars(), minLen);
        
        if (cmp < 0) return true;   // a vem antes de b
        if (cmp > 0) return false;  // b vem antes de a
        
        return a->length() < b->length();
        
    }
};
