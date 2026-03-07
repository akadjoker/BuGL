#include "code.hpp"
#include "value.hpp"
#include "string.hpp"

Code::Code(size_t capacity)
    : m_capacity(capacity), count(0)
{
    code = (uint8 *)aAlloc(capacity * sizeof(uint8));
    lines = (int *)aAlloc(capacity * sizeof(int));

    constants.reserve(1024);
    m_frozen = false;
    nilIndex = -1;
    trueIndex = -1;
    falseIndex = -1;
  
}

void Code::freeze()
{
    m_frozen = true;
}

 

int Code::addConstant(Value value)
{
    // 1. Tipos mutáveis - sempre  novo
    switch (value.type)
    {
        case ValueType::CLASSINSTANCE:
        case ValueType::NATIVECLASSINSTANCE:
        case ValueType::STRUCTINSTANCE:
        case ValueType::NATIVESTRUCTINSTANCE:
        case ValueType::ARRAY:
        case ValueType::MAP:
        case ValueType::POINTER:
        case ValueType::MODULEREFERENCE:
            constants.push(value);
            return static_cast<int>(constants.size() - 1);
        default:
            break;
    }
    
    // 2. Fast path para valores muito comuns
    if (value.type == ValueType::NIL)
    {
        if (nilIndex == -1)
        {
            nilIndex = constants.size();
            constants.push(value);
        }
        return nilIndex;
    }
    
    if (value.type == ValueType::BOOL)
    {
        if (value.asBool())
        {
            if (trueIndex == -1)
            {
                trueIndex = constants.size();
                constants.push(value);
            }
            return trueIndex;
        }
        else
        {
            if (falseIndex == -1)
            {
                falseIndex = constants.size();
                constants.push(value);
            }
            return falseIndex;
        }
    }
    
    // 3. Loop para outros tipos
    if (value.type == ValueType::STRING)
    {
        String *str = value.asString();
        for (int i = 0; i < constants.size(); i++)
        {
            if (constants[i].type == ValueType::STRING &&
                constants[i].asString() == str)  
            {
               // Warning("Constant already exists");
                return i;
            }
        }
    }
    else if (value.type == ValueType::INT)
    {
        for (int i = 0; i < constants.size(); i++)
        {
            if (constants[i].type == ValueType::INT &&
                constants[i].asInt() == value.asInt())
            {
              //  Warning("Constant already exists");
                return i;
            }
        }
    }
    else if (value.type == ValueType::DOUBLE)
    {
        for (int i = 0; i < constants.size(); i++)
        {
            if (constants[i].type == ValueType::DOUBLE &&
                constants[i].asDouble() == value.asDouble())
            {
              //  Warning("Constant already exists");
                return i;
            }
        }
    }
    else if (value.type == ValueType::CLASS || 
             value.type == ValueType::STRUCT || 
             value.type == ValueType::NATIVE  || 
             value.type == ValueType::FUNCTION ||
             value.type == ValueType::NATIVECLASS || 
             value.type == ValueType::PROCESS || 
             value.type == ValueType::NATIVESTRUCT
            )
    {
        for (int i = 0; i < constants.size(); i++)
        {
            if (constants[i].type == value.type &&
                constants[i].as.integer == value.as.integer)
            {
              // Warning("Constant already exists");
                return i;
            }
        }
    }
    
    // 4. Adiciona novo
   // Info("Adding constant");
    constants.push(value);
    return static_cast<int>(constants.size() - 1);
}


void Code::clear()
{
    if (code)
    {
        aFree(code);
        code = nullptr;
    }
    if (lines)
    {
        aFree(lines);
        lines = nullptr;
    }
    constants.destroy();
    m_capacity = 0;
    count = 0;
}

void Code::writeShort(uint16 value, int line)
{
    write((value >> 8) & 0xff, line);
    write(value & 0xff, line);
}

void Code::reserve(size_t capacity)
{
    if (capacity > m_capacity)
    {
        uint8 *newCode = (uint8 *)aRealloc(code, capacity * sizeof(uint8));
        if (!newCode)
            return;

        int *newLine = (int *)aRealloc(lines, capacity * sizeof(int));
        if (!newLine)
        {
            code = newCode;
            DEBUG_BREAK_IF(true);
            return;
        }

        code = newCode;
        lines = newLine;
        m_capacity = capacity;
    }
}

void Code::write(uint8 instruction, int line)
{
    DEBUG_BREAK_IF(m_frozen);
    if (m_capacity < count + 1)
    {
        size_t newCapacity = GROW_CAPACITY(m_capacity);
        reserve(newCapacity);
    }

    code[count] = instruction;
    lines[count] = line;
    count++;
}

uint8 Code::operator[](size_t index)
{
    DEBUG_BREAK_IF(index >= count);
    return code[index];
}

bool ValueEq::operator()(const Value &a, const Value &b) const
{
    {
        if (a.type != b.type)
            return false;

        switch (a.type)
        {
        case ValueType::NIL:
            return true;

        case ValueType::BOOL:
            return a.asBool() == b.asBool();

        case ValueType::INT:
            return a.asInt() == b.asInt();

        case ValueType::DOUBLE:
            return a.asDouble() == b.asDouble();

        case ValueType::STRING:
            return a.asString() == b.asString(); 

        case ValueType::CLASS:
        case ValueType::STRUCT:
        case ValueType::NATIVE:
            return a.as.integer == b.as.integer;

        default:
            return false;
        }
    }
}

size_t ValueHasher::operator()(const Value &v) const
{
    {
        switch (v.type)
        {
        case ValueType::NIL:
            return 0;
        case ValueType::BOOL:
            return v.asBool() ? 1 : 0;

        case ValueType::INT:
        {
            // MurmurHash3 finalizer (mix bits)
            uint32_t h = static_cast<uint32_t>(v.asInt());
            h ^= h >> 16;
            h *= 0x85ebca6b;
            h ^= h >> 13;
            h *= 0xc2b2ae35;
            h ^= h >> 16;
            return h;
        }

        case ValueType::DOUBLE:
        {
            // Reinterpret bits como uint64
            double d = v.asDouble();
            uint64_t bits;
            memcpy(&bits, &d, sizeof(double));
            // Hash as uint64
            bits ^= bits >> 33;
            bits *= 0xff51afd7ed558ccdULL;
            bits ^= bits >> 33;
            bits *= 0xc4ceb9fe1a85ec53ULL;
            bits ^= bits >> 33;
            return static_cast<size_t>(bits);
        }
        case ValueType::STRING:
            return v.asString()->hash;
        case ValueType::CLASS:
        case ValueType::STRUCT:
        case ValueType::NATIVE:
            return v.as.integer; // Id do objeto
        default:
            return 0; // Objetos não vão no cache
        }
    }
}