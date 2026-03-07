/**
 * @class Vector
 * @brief A high-performance vector container optimized for POD (Plain Old Data) types.
 * 
 * This template class provides a dynamic array implementation specifically designed
 * for POD types without constructors or destructors. It uses move semantics and
 * disables copying to ensure efficient memory management.
 * 
 * @tparam T The POD type stored in the vector. Must be a Plain Old Data type.
 * 
 * @note This class uses custom memory allocation functions (aAlloc/aFree).
 * @note Copy operations are explicitly deleted; only move semantics are supported.
 * @note All operations assume T is a POD type and use memcpy/memmove for efficiency.
 * 
 * @section Constructors
 * - Vector(): Default constructor with initial capacity of 8.
 * - Vector(size_t initialCapacity): Constructor with specified initial capacity.
 * - Vector(Vector&&) noexcept: Move constructor.
 * 
 * @section Memory Management
 * - reserve(size_t newCapacity): Ensures capacity is at least newCapacity.
 * - destroy(): Frees all allocated memory and resets state.
 * - resize(size_t newSize): Changes the logical size of the vector.
 * 
 * @section Element Operations
 * - push(const T&): Adds an element to the end.
 * - emplace(Args&&...): Constructs and adds an element to the end.
 * - pop(): Removes the last element.
 * - back(): Returns reference to the last element.
 * - insert(size_t index, const T&): Inserts element at specified index.
 * - remove(size_t index): Removes element at specified index.
 * 
 * @section Search and Utility
 * - find(const T&): Returns index of element or -1 if not found.
 * - contains(const T&): Checks if element exists in vector.
 * - reverse(): Reverses the order of elements.
 * - swap(size_t i, size_t j): Swaps two elements by index.
 * - clear(): Removes all elements without deallocating memory.
 * 
 * @section Accessors
 * - operator[](size_t): Direct element access by index.
 * - data(): Returns pointer to underlying data buffer.
 * - size(): Returns number of elements currently stored.
 * - capacity(): Returns total allocated capacity.
 * - empty(): Checks if vector contains no elements.
 * - begin(), end(): Iterator support for range-based loops.
 */
#pragma once
#include <cstdint>
#include <cstring>
#include <utility>  

// Vector otimizado para tipos POD (sem constructor/destructor)
template <typename T>
class Vector
{
private:
    T *data_;
    size_t size_;
    size_t capacity_;
 

public:
    Vector()
        : data_(nullptr), size_(0), capacity_(0)
    {
        reserve(8);
    }



    explicit Vector(size_t initialCapacity)
        : data_(nullptr), size_(0), capacity_(0)
    {
        reserve(initialCapacity);
    }

    ~Vector()
    {
        destroy();
    }

    // Move
    Vector(Vector &&other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    Vector &operator=(Vector &&other) noexcept
    {
        if (this != &other)
        {
            destroy();
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
     

            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    // Delete copy
    Vector(const Vector &) = delete;
    Vector &operator=(const Vector &) = delete;

    void destroy()
    {
        if (data_)
        {
            aFree(data_);
            data_ = nullptr;
            size_ = 0;
            capacity_ = 0;
        }
    }

    void reserve(size_t newCapacity)
    {
        if (newCapacity <= capacity_)
            return;

        // Aloca novo bloco
        T *newData = (T *)aAlloc(newCapacity * sizeof(T));

        // Copia dados antigos (POD - usa memcpy)
        if (data_)
        {
            std::memcpy(newData, data_, size_ * sizeof(T));
            aFree(data_);
        }

        data_ = newData;
        capacity_ = newCapacity;
    }

    void push(const T &value)
    {
        if (size_ >= capacity_)
        {
            //size_t newCap = capacity_ < 8 ? 8 : capacity_ * 2;
            size_t newCap=CalculateCapacityGrow(capacity_,size_+1);
            reserve(newCap);
        }

        data_[size_++] = value; // Simples assignment
    }

    template <typename... Args>
    void emplace(Args &&...args)
    {
        if (size_ >= capacity_)
        {
            //size_t newCap = capacity_ < 8 ? 8 : capacity_ * 2;
            size_t newCap=CalculateCapacityGrow(capacity_,size_+1);
            reserve(newCap);
        }

        // Para POD, isto funciona (aggregate initialization)
        data_[size_++] = T{std::forward<Args>(args)...};
    }
    void pop()
    {
        if (size_ > 0)
        {
            size_--;
        }
    }

    T &back()
    {
        return data_[size_ - 1];
    }

    const T &back() const
    {
        return data_[size_ - 1];
    }

    void clear()
    {
        size_ = 0; // Sem destructors!
    }

    void resize(size_t newSize)
    {
        if (newSize > capacity_)
        {
            reserve(newSize);
        }
        size_ = newSize;
    }

    // Accessors
    T &operator[](size_t i) { return data_[i]; }
    const T &operator[](size_t i) const { return data_[i]; }

    T *data() { return data_; }
    const T *data() const { return data_; }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    // Iterators
    T *begin() { return data_; }
    T *end() { return data_ + size_; }
    const T *begin() const { return data_; }
    const T *end() const { return data_ + size_; }


    void insert(size_t index, const T &value)
    {
        if (index > size_)
            index = size_;
        
        if (size_ >= capacity_)
        {
            size_t newCap = CalculateCapacityGrow(capacity_, size_ + 1);
            reserve(newCap);
        }
        
        // Move elementos para frente
        if (index < size_)
        {
            std::memmove(data_ + index + 1, data_ + index, (size_ - index) * sizeof(T));
        }
        
        data_[index] = value;
        size_++;
    }
    
    // REMOVE - Remove por índice
    void remove(size_t index)
    {
        if (index >= size_)
            return;
        
        // Move elementos para trás
        if (index < size_ - 1)
        {
            std::memmove(data_ + index, data_ + index + 1, (size_ - index - 1) * sizeof(T));
        }
        
        size_--;
    }
    
    // FIND - Retorna índice ou -1
    int find(const T &value) const
    {
        for (size_t i = 0; i < size_; i++)
        {
            if (memcmp(&data_[i], &value, sizeof(T)) == 0)
                return (int)i;
        }
        return -1;
    }
    
 
    bool contains(const T &value) const
    {
        return find(value) != -1;
    }
    
    // REVERSE - Inverte ordem
    void reverse()
    {
        for (size_t i = 0; i < size_ / 2; i++)
        {
            T temp = data_[i];
            data_[i] = data_[size_ - 1 - i];
            data_[size_ - 1 - i] = temp;
        }
    }
    
    // SWAP - Troca dois elementos
    void swap(size_t i, size_t j)
    {
        if (i >= size_ || j >= size_)
            return;
        
        T temp = data_[i];
        data_[i] = data_[j];
        data_[j] = temp;
    }

};