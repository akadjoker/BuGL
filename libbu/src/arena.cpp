#include "arena.hpp"
#include "utils.hpp"
#include <cstring>
#include <cstdlib>
#include <climits>

size_t HeapAllocator::s_blockSizes[blockSizes] =
	{
		16,	 // 0
		32,	 // 1
		64,	 // 2
		96,	 // 3
		128, // 4
		160, // 5
		192, // 6
		224, // 7
		256, // 8
		320, // 9
		384, // 10
		448, // 11
		512, // 12
		640, // 13
};
uint8 HeapAllocator::s_blockSizeLookup[maxBlockSize + 1];
bool HeapAllocator::s_blockSizeLookupInitialized;

struct Heap
{
	size_t blockSize;
	Block *blocks;
};

struct Block
{
	Block *next;
};

HeapAllocator::HeapAllocator()
{
	assert(blockSizes < UCHAR_MAX);

	m_totalAllocated = 0;
	m_totalReserved = 0;
	m_largeAllocations = 0;
	m_largeAllocatedBytes = 0;

	m_chunkSpace = chunkArrayIncrement;
	m_chunkCount = 0;
	m_chunks = (Heap *)aAlloc(m_chunkSpace * sizeof(Heap));

	std::memset(m_chunks, 0, m_chunkSpace * sizeof(Heap));
	for (auto &list : m_freeLists)
	{
		list = nullptr;
	}

	if (s_blockSizeLookupInitialized == false)
	{
		size_t j = 0;
		for (size_t i = 1; i <= maxBlockSize; ++i)
		{
			assert(j < blockSizes);
			if (i <= s_blockSizes[j])
			{
				s_blockSizeLookup[i] = (uint8)j;
			}
			else
			{
				++j;
				s_blockSizeLookup[i] = (uint8)j;
			}
		}

		s_blockSizeLookupInitialized = true;
	}
	std::memset(m_blockAllocations, 0, sizeof(m_blockAllocations));
}

HeapAllocator::~HeapAllocator()
{

	for (size_t i = 0; i < m_chunkCount; ++i)
	{
		aFree(m_chunks[i].blocks);
	}

	aFree(m_chunks);
}

void *HeapAllocator::Allocate(size_t size)
{
	if (size == 0)
		return NULL;

	assert(0 < size);

	if (size > maxBlockSize)
	{
		m_totalAllocated += size;
		m_largeAllocations++;
		m_largeAllocatedBytes += size;
		   Info("Large allocation: %zu bytes (bypassing arena!)", size);
		return aAlloc(size);
	}

	size_t index = s_blockSizeLookup[size];
	assert(0 <= index && index < blockSizes);
	size_t blockSize = s_blockSizes[index];

	if (m_freeLists[index])
	{
		Block *block = m_freeLists[index];
		m_freeLists[index] = block->next;
		m_totalAllocated += blockSize;
		m_blockAllocations[index]++;
		return block;
	}
	else
	{
		if (m_chunkCount == m_chunkSpace)
		{
			Heap *oldChunks = m_chunks;
			m_chunkSpace += chunkArrayIncrement;
			m_chunks = (Heap *)aAlloc(m_chunkSpace * sizeof(Heap));
			std::memcpy(m_chunks, oldChunks, m_chunkCount * sizeof(Heap));
			std::memset(m_chunks + m_chunkCount, 0, chunkArrayIncrement * sizeof(Heap));
			aFree(oldChunks);
		}

		Heap *chunk = m_chunks + m_chunkCount;
		chunk->blocks = (Block *)aAlloc(chunkSize);
#if defined(_DEBUG)
		std::memset(chunk->blocks, 0xcd, chunkSize);
#endif
		chunk->blockSize = blockSize;
		size_t blockCount = chunkSize / blockSize;
		assert(blockCount * blockSize <= chunkSize);
		auto *base = reinterpret_cast<int8 *>(chunk->blocks);
		for (size_t i = 0; i < blockCount - 1; ++i)
		{
			Block *block = reinterpret_cast<Block *>(base + static_cast<ptrdiff_t>(blockSize) * i);
			Block *next = reinterpret_cast<Block *>(base + static_cast<ptrdiff_t>(blockSize) * (i + 1));
			block->next = next;
		}
		Block *last = reinterpret_cast<Block *>(base + static_cast<ptrdiff_t>(blockSize) * (blockCount - 1));
		last->next = NULL;

		m_freeLists[index] = chunk->blocks->next;
		++m_chunkCount;

		m_totalReserved += chunkSize;
		m_totalAllocated += blockSize;
		m_blockAllocations[index]++;

		return chunk->blocks;
	}
}

void HeapAllocator::Free(void *p, size_t size)
{
	if (size == 0)
	{
		return;
	}

	assert(0 < size);

	if (size > maxBlockSize)
	{
		m_totalAllocated -= size;
		m_largeAllocations--;
		m_largeAllocatedBytes -= size;
		aFree(p);
		return;
	}

	size_t index = s_blockSizeLookup[size];
	assert(0 <= index && index < blockSizes);
	size_t blockSize = s_blockSizes[index];
	m_totalAllocated -= blockSize;
	m_blockAllocations[index]--;

#ifdef _DEBUG
	// Verify the memory address and size is valid.

	bool found = false;
	for (size_t i = 0; i < m_chunkCount; ++i)
	{
		Heap *chunk = m_chunks + i;
		if (chunk->blockSize != blockSize)
		{
			assert((int8 *)p + blockSize <= (int8 *)chunk->blocks ||
				   (int8 *)chunk->blocks + chunkSize <= (int8 *)p);
		}
		else
		{
			if ((int8 *)chunk->blocks <= (int8 *)p && (int8 *)p + blockSize <= (int8 *)chunk->blocks + chunkSize)
			{
				found = true;
			}
		}
	}

	assert(found);

	std::memset(p, 0xfd, blockSize);
#endif

	Block *block = (Block *)p;
	block->next = m_freeLists[index];
	m_freeLists[index] = block;
}

void HeapAllocator::Clear()
{
	m_totalAllocated = 0;
	m_totalReserved = 0;
	m_largeAllocations = 0;
	m_largeAllocatedBytes = 0;
	std::memset(m_blockAllocations, 0, sizeof(m_blockAllocations));
	for (size_t i = 0; i < m_chunkCount; ++i)
	{
		aFree(m_chunks[i].blocks);
	}

	m_chunkCount = 0;
	std::memset(m_chunks, 0, m_chunkSpace * sizeof(Heap));

	for (auto &list : m_freeLists)
	{
		list = nullptr;
	}
}

void HeapAllocator::GetStats(AllocationStats &stats) const
{
    stats.totalReserved = m_totalReserved;
    stats.totalAllocated = m_totalAllocated;
    
    // Corrigir: totalFree só considera memória de chunks
    // Não incluir large allocations no cálculo
    size_t allocatedInChunks = m_totalAllocated - m_largeAllocatedBytes;
    stats.totalFree = (m_totalReserved > allocatedInChunks) 
                      ? (m_totalReserved - allocatedInChunks) 
                      : 0;
    
    stats.chunkCount = m_chunkCount;
    stats.largeAllocations = m_largeAllocations;
    stats.largeAllocatedBytes = m_largeAllocatedBytes;

    std::memcpy(stats.blockStats, m_blockAllocations, sizeof(m_blockAllocations));
}

void HeapAllocator::Stats()
{
    size_t used = GetTotalAllocated();
	
    AllocationStats stats;
    GetStats(stats);
	
    Info("=== Heap Allocator Stats ===");
    Info("  Memory used: %s", formatBytes(used) );
    Info("  Reserved: %s ", formatBytes(stats.totalReserved));
    Info("  Allocated: %s ", formatBytes(stats.totalAllocated));
    

    if (stats.totalReserved > 0)
    {
        double utilization = 100.0 * (stats.totalAllocated - stats.largeAllocatedBytes) / stats.totalReserved;
        Info("  Free: %s  (%.1f%% utilization)", 
             formatBytes(stats.totalFree), 
             utilization);
    }
    else
    {
        Info("  Free: 0 KB (no chunks allocated)");
    }
    
    Info("  Chunks: %zu", stats.chunkCount);
    Info("  Large allocs: %zu (%s)", 
         stats.largeAllocations, 
         formatBytes(stats.largeAllocatedBytes));

    if (blockSizes)
    {
		if(stats.blockStats[0] > 0)
        	Info("=== Block Distribution ===");
        for (size_t i = 0; i < blockSizes; i++)
        {
            if (stats.blockStats[i] > 0)
            {
                Info("* Index: %zu  %3zu bytes: %4zu allocs (%s)",i,
                     s_blockSizes[i],
                     stats.blockStats[i],
                     formatBytes((s_blockSizes[i] * stats.blockStats[i])));
            }
        }
    }
}

//********************************************************************** */

StackAllocator::StackAllocator()
{
	m_index = 0;
	m_allocation = 0;
	m_maxAllocation = 0;
	m_entryCount = 0;
}

StackAllocator::~StackAllocator()
{
	assert(m_index == 0);
	assert(m_entryCount == 0);
}

void *StackAllocator::Allocate(size_t size)
{
	assert(m_entryCount < maxStackEntries);

	StackEntry *entry = m_entries + m_entryCount;
	entry->size = size;
	if (m_index + size > stackSize)
	{
		entry->data = (char *)aAlloc(size);
		entry->usedMalloc = true;
	}
	else
	{
		entry->data = m_data + m_index;
		entry->usedMalloc = false;
		m_index += size;
	}

	m_allocation += size;
	m_maxAllocation = Max(m_maxAllocation, m_allocation);
	++m_entryCount;

	return entry->data;
}

void StackAllocator::Free(void *p)
{
	assert(m_entryCount > 0);
	StackEntry *entry = m_entries + m_entryCount - 1;
	assert(p == entry->data);
	if (entry->usedMalloc)
	{
		aFree(p);
	}
	else
	{
		m_index -= entry->size;
	}
	m_allocation -= entry->size;
	--m_entryCount;

	p = NULL;
}

size_t StackAllocator::GetMaxAllocation() const
{
	return m_maxAllocation;
}