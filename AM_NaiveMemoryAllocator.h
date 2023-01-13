#pragma once
#include "AM_SimpleMemoryBlock.h"
#include "VkDrawContext.h"

// TODO: 
// 1. memoryBlock for staging buffers
// 2. optimize with single buffer for each memory type
// 3. improve alloc overhead and merge empty slots

// TODO: concurrency
class AM_NaiveMemoryAllocator
{
public:
	AM_NaiveMemoryAllocator() = default;
	~AM_NaiveMemoryAllocator()
	{
		FreeVkDeviceMemory();
	}
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator& anAllocator) = delete;
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator&& anAllocator) = delete;

	void Init(uint32_t aMemoryTypeCount);
	[[nodiscard]] AM_SimpleMemoryObject& Allocate(const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements);
	void FreeVkDeviceMemory();

private:
	[[nodiscard]] AM_SimpleMemoryBlock& CreateAndGetNewBlock(const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_SimpleMemoryObject* TryGetFreeSlot(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements);
	std::vector<std::vector<AM_SimpleMemoryBlock>> myMemoryBlocksByMemoryType;
};

