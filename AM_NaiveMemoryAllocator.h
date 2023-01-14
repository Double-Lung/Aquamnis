#pragma once
#include "AM_SimpleMemoryBlock.h"
#include "VkDrawContext.h"

// TODO: 
// 1. optimize with single buffer for each memory type
// 2. improve alloc overhead and merge empty slots

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
	[[nodiscard]] AM_SimpleMemoryObject& AllocateUniformBufferMemory(void** outMappedMemory, const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements)
	{
		return AllocateMappedBufferMemory(outMappedMemory, myUniformBufferMemoryBlock, aMemoryTypeIndex, aMemoryRequirements);
	}
	[[nodiscard]] AM_SimpleMemoryObject& AllocateStagingBufferMemory(void** outMappedMemory, const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements)
	{
		return AllocateMappedBufferMemory(outMappedMemory, myStagingBufferMemoryBlock, aMemoryTypeIndex, aMemoryRequirements);
	}

	void FreeVkDeviceMemory();

private:
	void AllocateMemory(VkDeviceMemory& outMemoryPtr, const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_SimpleMemoryObject& AllocateMappedBufferMemory(void** outMappedMemory, AM_SimpleMemoryBlock& aMemoryBlock, const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements);
	[[nodiscard]] AM_SimpleMemoryObject* SubAllocation(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements);
	[[nodiscard]] inline uint64_t GetPadding(const uint64_t anOffset, const uint64_t anAlignmentSize) const;
	[[nodiscard]] AM_SimpleMemoryObject* TryGetFreeSlot(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements);
	std::vector<std::vector<AM_SimpleMemoryBlock>> myMemoryBlocksByMemoryType;
	AM_SimpleMemoryBlock myUniformBufferMemoryBlock;
	AM_SimpleMemoryBlock myStagingBufferMemoryBlock;
};

