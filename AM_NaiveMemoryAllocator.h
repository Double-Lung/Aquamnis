#pragma once
#include "AM_SimpleMemoryBlock.h"
#include <unordered_map>
#include "AM_VkContext.h"
#include "AM_BufferMemoryBlock.h"
#include "AM_ImageMemoryBlock.h"

// TODO: 
// merge empty slots
// concurrency

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
	[[nodiscard]] AM_Buffer* AllocateBuffer(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty);
	[[nodiscard]] AM_Buffer* AllocateMappedBuffer(const uint64_t aSize, const VkBufferUsageFlags aUsage);
	AM_Image& AllocateImageMemory(const uint32_t aMemoryTypeIndex, const uint64_t aSize){}
	void CopyToMappedMemory(AM_Buffer& aBuffer, void* aSource, const size_t aSize);

	void FreeVkDeviceMemory();

private:
	struct MemoryRequirements
	{
		uint64_t myAlignment = 0;
		uint32_t myMemoryTypeIndex = 0;
	};

	struct MemoryPropertyCache
	{
		VkFlags myMemoryProperty = 0;
		std::unordered_map<VkFlags, MemoryRequirements> myMemReqByBufferUsage;
	};

	void AllocateMemory(VkDeviceMemory& outMemoryPtr, const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_SimpleMemoryObject* SubAllocation(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements);
	[[nodiscard]] inline uint64_t GetPadding(const uint64_t anOffset, const uint64_t anAlignmentSize) const;
	[[nodiscard]] AM_SimpleMemoryObject* TryGetFreeSlot(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements);

	[[nodiscard]] AM_Buffer* AllocateBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty, MemoryPropertyCache& aCache);
	[[nodiscard]] AM_Buffer* AllocateBufferFast(const uint64_t aSize, const MemoryRequirements& aRequirement);
	[[nodiscard]] AM_Buffer* AllocateBufferSlow(const uint64_t aSize, const MemoryRequirements& aRequirement);

	[[nodiscard]] AM_Buffer* AllocateMappedBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty, MemoryPropertyCache& aCache);
	[[nodiscard]] AM_Buffer* AllocateMappedBufferFast(const uint64_t aSize, const MemoryRequirements& aRequirement);
	[[nodiscard]] AM_Buffer* AllocateMappedBufferSlow(const uint64_t aSize, const MemoryRequirements& aRequirement);

	uint32_t FindMemoryTypeIndex(const uint32_t someMemoryTypeBits, const VkMemoryPropertyFlags someProperties) const;

	std::vector<std::vector<AM_SimpleMemoryBlock>> myMemoryBlocksByMemoryType;
	std::vector<std::vector<AM_BufferMemoryBlock>> myBufferMemoryPool;
	std::vector<std::vector<AM_ImageMemoryBlock>> myImageMemoryPool;
	std::vector<MemoryPropertyCache> myMemPropCache;
	VkPhysicalDeviceMemoryProperties myPhysicalMemoryProperties;
};

