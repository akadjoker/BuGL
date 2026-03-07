#include "string.hpp"
#include "pool.hpp"
#include "value.hpp"
#include "arena.hpp"
#include "interpreter.hpp"
#include "utils.hpp"

#include <ctype.h>
#include <new>
#include <stdarg.h>

StringPool::StringPool()
{
    bytesAllocated = 0;
    dummyString = allocString();
    size_t len = 0;
    const char *str = "NULL";
    dummyString->length_and_flag = len;
    dummyString->ptr = nullptr;
    std::memcpy(dummyString->data, str, len);
    dummyString->data[len] = '\0';
    dummyString->index = -1;
    dummyString->hash = hashString(dummyString->chars(), len);
    bytesAllocated += sizeof(String) + len;
}

StringPool::~StringPool()
{
}

// ============= STRING ALLOC =============

String *StringPool::allocString()
{
    void *mem = allocator.Allocate(sizeof(String));
    String *s = new (mem) String();
    return s;
}

void StringPool::deallocString(String *s)
{
    if (!s)
        return;

    //    Info("Dealloc string %p", s);
    bytesAllocated -= sizeof(String) + s->length() + 1;

    if (s->isLong() && s->ptr)
        allocator.Free(s->ptr, s->length() + 1);

    s->~String();
    allocator.Free(s, sizeof(String));
}

void StringPool::clear()
{
    Info("String pool clear %d strings", map.size());
    Info("String Pool cllocated %s bytes", formatBytes(bytesAllocated));

    for (size_t i = 0; i < map.size(); i++)
    {
        String *s = map[i];

        deallocString(s);
    }

    dummyString->~String();
    allocator.Free(dummyString, sizeof(String));

   // allocator.Stats();
    allocator.Clear();

    map.clear();
    pool.destroy();
}

String *StringPool::create(const char *str, uint32 len)
{
    // Cache hit?

    int index = 0;

    if (pool.get(str, &index))
    {
        String *s = map[index];
        return s;
    }

    // New string
    String *s = allocString();

    // Copy data
    if (len <= String::SMALL_THRESHOLD)
    {
        s->length_and_flag = len;
        std::memcpy(s->data, str, len);
        s->data[len] = '\0';
    }
    else
    {
        s->length_and_flag = len | String::IS_LONG_FLAG;
        s->ptr = (char *)allocator.Allocate(len + 1);
        std::memcpy(s->ptr, str, len);
        s->ptr[len] = '\0';
    }

    s->hash = hashString(s->chars(), len);
    bytesAllocated += sizeof(String) + len;
    s->index = map.size();

    // Info("Create string %s hash %d len %d", s->chars(), s->hash, s->length());
    map.push(s);
    pool.set(s->chars(), map.size() - 1);

    // Store in pool

    return s;
}

String *StringPool::create(const char *str)
{
    return create(str, std::strlen(str));
}
// ========================================
// CONCAT - OTIMIZADO
// ========================================

String *StringPool::concat(String *a, String *b)
{
    size_t lenA = a->length();
    size_t lenB = b->length();

    // Fast paths
    if (lenA == 0)
        return b;
    if (lenB == 0)
        return a;

    size_t totalLen = lenA + lenB;

    //  USA ALLOCA para buffer temporário
    char *temp = (char *)alloca(totalLen + 1);
    std::memcpy(temp, a->chars(), lenA);
    std::memcpy(temp + lenA, b->chars(), lenB);
    temp[totalLen] = '\0';

    //  Cria string (com interning!)
    return create(temp, totalLen);
}

// ========================================
// UPPER/LOWER - OTIMIZADO
// ========================================

String *StringPool::upper(String *src)
{
    if (!src)
        return create("", 0);

    size_t len = src->length();

    //  ALLOCA buffer temporário
    char *temp = (char *)alloca(len + 1);

    const char *str = src->chars();
    for (size_t i = 0; i < len; i++)
    {
        temp[i] = (char)toupper((unsigned char)str[i]);
    }
    temp[len] = '\0';

    return create(temp, len);
}

String *StringPool::lower(String *src)
{
    if (!src)
        return create("", 0);

    size_t len = src->length();

    char *temp = (char *)alloca(len + 1);

    const char *str = src->chars();
    for (size_t i = 0; i < len; i++)
    {
        temp[i] = (char)tolower((unsigned char)str[i]);
    }
    temp[len] = '\0';

    return create(temp, len);
}

// ========================================
// SUBSTRING
// ========================================

String *StringPool::substring(String *src, uint32 start, uint32 end)
{
    if (!src)
        return create("", 0);

    size_t len = src->length();

    if (start >= len)
        start = len;
    if (end > len)
        end = len;
    if (start > end)
        start = end;

    size_t newLen = end - start;
    if (newLen == 0)
        return create("", 0);

    char *temp = (char *)alloca(newLen + 1);
    std::memcpy(temp, src->chars() + start, newLen);
    temp[newLen] = '\0';

    return create(temp, newLen);
}

// ========================================
// REPLACE -
// ========================================

String *StringPool::replace(String *src, const char *oldStr, const char *newStr)
{
    if (!src || !oldStr || !newStr)
        return src;

    const char *str = src->chars();
    size_t len = src->length();
    size_t oldLen = strlen(oldStr);
    size_t newLen = strlen(newStr);

    if (oldLen == 0)
        return src;

    // Conta ocorrências
    size_t count = 0;
    const char *pos = str;
    while ((pos = strstr(pos, oldStr)) != nullptr)
    {
        count++;
        pos += oldLen;
    }

    if (count == 0)
        return src;

    // Calcula tamanho final
    size_t finalLen = len - (count * oldLen) + (count * newLen);

    //  ALLOCA buffer temporário
    char *temp = (char *)alloca(finalLen + 1);

    // Copia com substituições
    const char *current = str;
    size_t destIdx = 0;

    while ((pos = strstr(current, oldStr)) != nullptr)
    {
        size_t copyLen = pos - current;
        std::memcpy(temp + destIdx, current, copyLen);
        destIdx += copyLen;

        std::memcpy(temp + destIdx, newStr, newLen);
        destIdx += newLen;

        current = pos + oldLen;
    }

    // Copia resto
    size_t remainLen = len - (current - str);
    std::memcpy(temp + destIdx, current, remainLen);
    temp[finalLen] = '\0';

    return create(temp, finalLen);
}

// ========================================
// AT -
// ========================================

String *StringPool::at(String *str, int index)
{
    if (!str)
        return create("", 0);

    int len = str->length();

    // Python-style negative indexing
    if (index < 0)
        index += len;

    if (index < 0 || index >= len)
        return create("", 0);

    char buf[2] = {str->chars()[index], '\0'};
    return create(buf, 1);
}

// ========================================
// CONTAINS/STARTSWITH/ENDSWITH -
// ========================================

bool StringPool::contains(String *str, String *substr)
{
    if (!str || !substr)
        return false;
    if (substr->length() == 0)
        return true;

    return strstr(str->chars(), substr->chars()) != nullptr;
}

bool StringPool::startsWith(String *str, String *prefix)
{
    if (!str || !prefix)
        return false;
    if (prefix->length() > str->length())
        return false;

    return strncmp(str->chars(), prefix->chars(), prefix->length()) == 0;
}

bool StringPool::endsWith(String *str, String *suffix)
{
    if (!str || !suffix)
        return false;

    int strLen = str->length();
    int suffixLen = suffix->length();

    if (suffixLen > strLen)
        return false;

    return strcmp(str->chars() + (strLen - suffixLen),
                  suffix->chars()) == 0;
}

// ========================================
// TRIM
// ========================================

String *StringPool::trim(String *str)
{
    if (!str)
        return create("", 0);

    const char *start = str->chars();
    const char *end = start + str->length() - 1;

    while (*start && isspace((unsigned char)*start))
        start++;

    while (end > start && isspace((unsigned char)*end))
        end--;

    if (end < start)
        return create("", 0);

    size_t len = end - start + 1;

    //  create() já lida com substring não null-terminated
    char *temp = (char *)alloca(len + 1);
    std::memcpy(temp, start, len);
    temp[len] = '\0';

    return create(temp, len);
}

// ========================================
// INDEXOF -
// ========================================

int StringPool::indexOf(String *str, String *substr, int startIndex)
{
    if (!str || !substr)
        return -1;
    if (substr->length() == 0)
        return startIndex;

    int strLen = str->length();

    if (startIndex < 0)
        startIndex = 0;
    if (startIndex >= strLen)
        return -1;

    const char *start = str->chars() + startIndex;
    const char *found = strstr(start, substr->chars());

    if (!found)
        return -1;

    return (int)(found - str->chars());
}

int StringPool::indexOf(String *str, const char *substr, int startIndex)
{
    if (!str || !substr)
        return -1;

    //  OTIMIZAÇÃO: Evita criar String se não encontrar
    const char *start = str->chars() + (startIndex < 0 ? 0 : startIndex);
    const char *found = strstr(start, substr);

    if (!found)
        return -1;

    return (int)(found - str->chars());
}

// ========================================
// REPEAT
// ========================================

String *StringPool::repeat(String *str, int count)
{
    if (!str || count <= 0)
        return create("", 0);
    if (count == 1)
        return str;

    size_t len = str->length();
    size_t totalLen = len * count;

    //  ALLOCA se não for muito grande
    char *temp;
    bool useAlloca = (totalLen < 4096); // Stack limit

    if (useAlloca)
    {
        temp = (char *)alloca(totalLen + 1);
    }
    else
    {
        temp = (char *)allocator.Allocate(totalLen + 1);
    }

    // Repete
    for (int i = 0; i < count; i++)
    {
        std::memcpy(temp + (i * len), str->chars(), len);
    }
    temp[totalLen] = '\0';

    String *result = create(temp, totalLen);

    if (!useAlloca)
    {
        allocator.Free(temp, totalLen + 1);
    }

    return result;
}

// ========================================
// TOSTRING -
// ========================================

String *StringPool::toString(int value)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "%d", value);
    return create(buf);
}

String *StringPool::toString(uint32 value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    return create(buf);
}

String *StringPool::toString(double value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", value);
    return create(buf);
}

// ========================================
// FORMAT - OTIMIZADO
// ========================================

String *StringPool::format(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *buffer = (char *)alloca(4096);

    int len = vsnprintf(buffer, 4096, fmt, args);
    va_end(args);

    if (len < 0 || len >= 4096)
    {
        // Fallback: heap allocation
        va_start(args, fmt);
        int needed = vsnprintf(nullptr, 0, fmt, args);
        va_end(args);

        if (needed < 0)
            return create("", 0);

        char *heap = (char *)aAlloc(needed + 1);

        va_start(args, fmt);
        vsnprintf(heap, needed + 1, fmt, args);
        va_end(args);

        String *result = create(heap, needed);
        aFree(heap);

        return result;
    }

    return create(buffer, len);
}

String *StringPool::getString(int index)
{
    if (index < 0 || index >= map.size())
    {
        Warning("String index out of bounds: %d", index);
        return dummyString;
    }
    return map[index];
}

// ========================================
// SPLIT
// ========================================
 

// bool StringPool::split(String* str, String* separator, ArrayInstance *result)
// {
//     Vector<String*> result;

//     if (!str) return result;

//     const char* strChars = str->chars();
//     int strLen = str->length();

//     // CASO 1: Separador Vazio ou Nulo ("")
//     // Comportamento: Divide caractere a caractere ["a", "b", "c"]
//     if (!separator || separator->length() == 0)
//     {
//         result->values.reserve(strLen); // Otimiza alocação
//         for (int i = 0; i < strLen; i++)
//         {
//             // Cria string de 1 char 
//             char buf[2] = {strChars[i], '\0'};
//             result->values.push(create(buf, 1));
//         }
//         return result;
//     }

//     // CASO 2: Split Normal
//     const char* sepChars = separator->chars();
//     int sepLen = separator->length();

//     const char* start = strChars;
//     const char* end = strChars + strLen;
//     const char* current = start;
//     const char* found = nullptr;

//     // Loop usando strstr (nativo do C, muito rápido)
//     while ((found = strstr(current, sepChars)) != nullptr)
//     {
//         int partLen = found - current;
        
//         // create() já trata de alocar e internar a string
//         result.push(create(current, partLen));
        
//         // Avança ponteiro
//         current = found + sepLen;
//     }

//     // Adiciona o que sobrou da string (após o último separador)
//     // Ex: "a,b,c" -> após o último "b," sobra "c"
//     int remaining = end - current;
//     if (remaining >= 0)
//     {
//         result.push(create(current, remaining));
//     }

//     return result;
// }

ProcessPool::ProcessPool()
{
    pool.reserve(1000);
}

ProcessPool::~ProcessPool()
{
}

Process *ProcessPool::create()
{
    Process *proc = nullptr;

    if (pool.size() == 0)
    {
        proc = new Process();
    }
    else
    {
        proc = pool.back();
        pool.pop();
    }
    return proc;
}

void ProcessPool::recycle(Process *proc)
{
    proc->reset();
    pool.push(proc);
}

void ProcessPool::destroy(Process *proc)
{
    delete proc;
}

void ProcessPool::clear()
{
    //  Warning("Freeing %zu processes on pool", pool.size());

    for (size_t j = 0; j < pool.size(); j++)
    {
        Process *proc = pool[j];
        delete proc;
    }
    pool.clear();
}

void ProcessPool::shrink()
{
    if (pool.size() <= MIN_POOL_SIZE)
    {
        return; // Já está pequeno
    }


    int targetSize = MIN_POOL_SIZE + (pool.size() - MIN_POOL_SIZE) / 2;

    while (pool.size() > targetSize)
    {
        Process *proc = pool.back();
        pool.pop();
        delete proc;
    }
}
