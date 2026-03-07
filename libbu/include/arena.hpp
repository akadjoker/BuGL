

#pragma once
#include "config.hpp"

const size_t chunkSize = 16UL * 1024UL; // 16384 bytes
const size_t maxBlockSize = 640UL;
const size_t blockSizes = 14UL;
const size_t chunkArrayIncrement = 128UL;
const size_t stackSize = 100UL * 1024UL; // 100k
const size_t maxStackEntries = 32UL;

struct AllocationStats
{
	size_t totalReserved;		// Memória total reservada (chunks)
	size_t totalAllocated;		// Memória atualmente em uso
	size_t totalFree;			// Memória disponível nas freeLists
	size_t chunkCount;			// Número de chunks alocados
	size_t largeAllocations;	// Número de alocações > maxBlockSize
	size_t largeAllocatedBytes; // Bytes em alocações grandes

	size_t blockStats[blockSizes]; // Alocações ativas por tamanho
};

struct StackEntry
{
	char *data;
	size_t size;
	bool usedMalloc;
};

struct Block;
struct Heap;

//  StringObject *str = strings[i];
// str->~StringObject();
// ARENA_FREE(str, sizeof(StringObject));
// void *p = ARENA_ALLOC(sizeof(StringObject));
// StringObject *obj = new (p) StringObject(value);

class HeapAllocator
{
public:
	HeapAllocator();
	~HeapAllocator();

	HeapAllocator(const HeapAllocator &) = delete;
	HeapAllocator &operator=(const HeapAllocator &) = delete;
	HeapAllocator(HeapAllocator &&) = delete;
	HeapAllocator &operator=(HeapAllocator &&) = delete;

	/// Allocate memory. if the size is larger than maxBlockSize.
	void *Allocate(size_t size);

	void Free(void *p, size_t size);

	void Clear();

	void Stats();



	void GetStats(AllocationStats &stats) const;
	size_t GetTotalAllocated() const { return m_totalAllocated; }
	size_t GetTotalReserved() const { return m_totalReserved; }

private:
	Heap *m_chunks;
	size_t m_chunkCount;
	size_t m_chunkSpace;

	size_t m_totalAllocated;			   // Bytes atualmente alocados
	size_t m_totalReserved;				   // Bytes reservados em chunks
	size_t m_largeAllocations;			   // Count de malloc direto
	size_t m_largeAllocatedBytes;		   // Bytes em malloc direto
	size_t m_blockAllocations[blockSizes]; // Count por block size

	Block *m_freeLists[blockSizes];

	static size_t s_blockSizes[blockSizes];
	static uint8 s_blockSizeLookup[maxBlockSize + 1];
	static bool s_blockSizeLookupInitialized;
};

// This is a stack allocator used for fast per step allocations.
// You must nest allocate/free pairs. The code will assert
// if you try to interleave multiple allocate/free pairs.
class StackAllocator
{
public:
	StackAllocator();
	~StackAllocator();

	StackAllocator(const StackAllocator &) = delete;
	StackAllocator &operator=(const StackAllocator &) = delete;
	StackAllocator(StackAllocator &&) = delete;
	StackAllocator &operator=(StackAllocator &&) = delete;
	void *Allocate(size_t size);
	void Free(void *p);

	size_t GetMaxAllocation() const;

private:
	char m_data[stackSize];
	size_t m_index;

	size_t m_allocation;
	size_t m_maxAllocation;

	StackEntry m_entries[maxStackEntries];
	size_t m_entryCount;
};