#pragma once
#include "AM_SimpleMemoryBlock.h"
#include <unordered_map>
#include "VkDrawContext.h"
#include "AM_BufferMemoryBlock.h"
#include "AM_ImageMemoryBlock.h"

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

	void Init(const VkPhysicalDeviceMemoryProperties& aPhysicalMemoryProperties);
	[[nodiscard]] AM_SimpleMemoryObject& Allocate(const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements);
	[[nodiscard]] AM_SimpleMemoryObject& AllocateUniformBufferMemory(void** outMappedMemory, const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements)
	{
		return AllocateMappedBufferMemory(outMappedMemory, myUniformBufferMemoryBlock, aMemoryTypeIndex, aMemoryRequirements);
	}
	[[nodiscard]] AM_SimpleMemoryObject& AllocateStagingBufferMemory(void** outMappedMemory, const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements)
	{
		return AllocateMappedBufferMemory(outMappedMemory, myStagingBufferMemoryBlock, aMemoryTypeIndex, aMemoryRequirements);
	}

	[[nodiscard]] AM_Buffer* AllocateBuffer(const uint64_t aSize, VkBufferUsageFlags aUsage);
	[[nodiscard]] AM_Buffer* AllocateMappedBuffer(const uint64_t aSize, VkBufferUsageFlags aUsage);

	[[nodiscard]] AM_Buffer* AllocateBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage);
	[[nodiscard]] AM_Buffer* AllocateBufferFast(const uint64_t aSize, const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_Buffer* AllocateBufferSlow(const uint64_t aSize, const uint32_t aMemoryTypeIndex);

	AM_Image& AllocateImageMemory(const uint32_t aMemoryTypeIndex, const uint64_t aSize){}

	void FreeVkDeviceMemory();

private:
	void AllocateMemory(VkDeviceMemory& outMemoryPtr, const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_SimpleMemoryObject& AllocateMappedBufferMemory(void** outMappedMemory, AM_SimpleMemoryBlock& aMemoryBlock, const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements);
	[[nodiscard]] AM_SimpleMemoryObject* SubAllocation(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements);
	[[nodiscard]] inline uint64_t GetPadding(const uint64_t anOffset, const uint64_t anAlignmentSize) const;
	[[nodiscard]] AM_SimpleMemoryObject* TryGetFreeSlot(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements);


	uint32_t FindMemoryTypeIndex(const uint32_t memoryTypeBits, VkMemoryPropertyFlags const properties) const;


	std::vector<std::vector<AM_SimpleMemoryBlock>> myMemoryBlocksByMemoryType;
	AM_SimpleMemoryBlock myUniformBufferMemoryBlock;
	AM_SimpleMemoryBlock myStagingBufferMemoryBlock;

	std::unordered_map<uint32_t, std::vector<AM_BufferMemoryBlock>> myBufferMemoryPool;
	std::unordered_map<uint32_t, std::vector<AM_ImageMemoryBlock>> myImageMemoryPool;
	std::unordered_map<VkFlags, uint32_t> myBufferUsageToMemTypeIndex;
	std::unordered_map<VkFlags, uint32_t> myImageUsageToMemTypeIndex;
	VkPhysicalDeviceMemoryProperties myPhysicalMemoryProperties;
};

