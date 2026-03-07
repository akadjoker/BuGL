#include "array.hpp"
#include "utils.hpp"


void Array::destroy()
{
    if (data  )
    {
        aFree(data);
        data = nullptr;
        count = capacity = 0;
    }
}

void Array::reserve(size_t newCap)
{
    if (newCap <= capacity)
        return;

    // Aloca novo bloco
    Value *newData = (Value *)aAlloc(newCap * sizeof(Value));

    // Copia dados antigos
    if (data)
    {
        std::memcpy(newData, data, count * sizeof(Value));
        aFree(data);
    }

    data = newData;
    capacity = newCap;
}

void Array::push(Value v)
{
    if (count >= capacity)
    {
        size_t newCap = capacity < 8 ? 8 : capacity * 2;
        reserve(newCap);
    }
    data[count++] = v;
}

Value Array::pop()
{
    assert(count > 0);
    return data[--count];
}

Value Array::back()
{
    assert(count > 0);
    return data[count - 1];
}

const Value &Array::back() const
{
    assert(count > 0);
    return data[count - 1];
}

Value &Array::operator[](size_t i)
{
 
    return data[i];
}

const Value &Array::operator[](size_t i) const
{
 
    return data[i];
}

size_t Array::size() const
{
    return count;
}
