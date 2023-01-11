#pragma once

#include <vector>
#include "AM_SimpleMemoryBlock.h"

// TODO: 
// 1. deconstruction & memory leak fix
// 2. memoryBlock for staging buffers
// 3. optimize with single buffer for each memory type
// 4. improve alloc overhead and merge empty slots

// TODO: concurrency
class AM_NaiveMemoryAllocator
{
public:
	AM_NaiveMemoryAllocator() = default;
	~AM_NaiveMemoryAllocator() = default;
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator& anAllocator) = delete;
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator&& anAllocator) = delete;

	void Init(uint32_t aMemoryTypeCount);
	[[nodiscard]] AM_SimpleMemoryObject& Allocate(const uint32_t aMemoryTypeIndex, const uint64_t aSize);
	void DeleteMemoryBlocks();

private:
	[[nodiscard]] AM_SimpleMemoryBlock& CreateAndGetNewBlock(const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_SimpleMemoryObject* TryGetFreeSlot(AM_SimpleMemoryBlock& aMemoryBlock, const uint64_t aSize);

	std::vector<std::vector<AM_SimpleMemoryBlock>> myMemoryBlocksByMemoryType;
};

