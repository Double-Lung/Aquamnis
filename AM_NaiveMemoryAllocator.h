#pragma once

#include <vector>
#include "AM_SimpleMemoryBlock.h"

// TODO: free slot
// TODO: deconstruction
// TODO: consolidate empty slots
// TODO: concurrency

class AM_NaiveMemoryAllocator
{
public:
	AM_NaiveMemoryAllocator() = default;
	~AM_NaiveMemoryAllocator();;
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator& anAllocator) = delete;
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator&& anAllocator) = delete;

	void Init(uint32_t aMemoryTypeCount);
	[[nodiscard]] AM_SimpleMemoryObject& Allocate(const uint32_t aMemoryTypeIndex, const uint64_t aSize);

private:
	[[nodiscard]] AM_SimpleMemoryBlock& CreateAndGetNewBlock(const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_SimpleMemoryBlock& GetMemoryBlock(const uint32_t aMemoryTypeIndex, const uint64_t aSize);
	[[nodiscard]] AM_SimpleMemoryObject& SubAllocate(AM_SimpleMemoryBlock& aMemoryBlock, const uint64_t aSize);

	void FreeMemoryBlocks();
	std::vector<std::vector<AM_SimpleMemoryBlock>> myMemoryBlocksByMemoryType;
};

