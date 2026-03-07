#pragma once
#include "config.hpp"
#include "string.hpp"
#include "vector.hpp"
#include "map.hpp"
#include "types.hpp"

struct Value;
struct Process;

struct CStringHash
{
    size_t operator()(const char *str) const
    {
        // FNV-1a hash
        size_t hash = 2166136261u;
        while (*str)
        {
            hash ^= (unsigned char)*str++;
            hash *= 16777619u;
        }
        return hash;
    }
};

// Eq para const char*
struct CStringEq
{
    bool operator()(const char *a, const char *b) const
    {
        return strcmp(a, b) == 0;
    }
};

class StringPool
{
private:
    HeapAllocator allocator;

    HashMap<const char *, int, CStringHash, CStringEq> pool;
    size_t bytesAllocated = 0;
    friend class Interpreter;
    String *dummyString = nullptr;

    Vector<String *> map;
    String *allocString();
    void deallocString(String *s);

public:
    StringPool();
    ~StringPool();

    size_t getBytesAllocated() { return bytesAllocated; }

    String *create(const char *str, uint32 len);
    void destroy(String *s);

    String *create(const char *str);

    String *format(const char *fmt, ...);

    String *getString(int index);

    int indexOf(String *str, String *substr, int startIndex = 0);
    int indexOf(String *str, const char *substr, int startIndex = 0);

    String *concat(String *a, String *b);
    String *upper(String *src);
    String *lower(String *src);
    String *substring(String *src, uint32 start, uint32 end);
    String *replace(String *src, const char *oldStr, const char *newStr);
    

    String *to_string(Value v);

    String *trim(String *str);
    bool contains(String *str, String *substr);
    bool startsWith(String *str, String *prefix);
    bool endsWith(String *str, String *suffix);
    String *at(String *str, int index);
    String *repeat(String *str, int count);

    String *toString(int value);
    String *toString(uint32 value);
    String *toString(double value);

    void clear();
};

class ProcessPool
{

    Vector<Process *> pool;

public:
    ProcessPool();
    ~ProcessPool();
    static const int MAX_POOL_SIZE = 128;     // Máximo no pool
    static const int MIN_POOL_SIZE = 32;      // Mínimo a manter
    static const int CLEANUP_THRESHOLD = 256; // Trigger cleanup

    static ProcessPool &instance()
    {
        static ProcessPool pool;
        return pool;
    }

    Process *create();
    void destroy(Process *proc);
    void recycle(Process *proc);
    void clear();
    void shrink();
    size_t size() const { return pool.size(); }
};

inline bool compareString(String *a, String *b)
{
    return compare_strings(a, b);
}
