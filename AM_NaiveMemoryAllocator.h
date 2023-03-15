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
	~AM_NaiveMemoryAllocator() = default;
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator& anAllocator) = delete;
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator&& anAllocator) = delete;

	void Init(const VkPhysicalDeviceMemoryProperties& aPhysicalMemoryProperties);
	[[nodiscard]] AM_Buffer* AllocateBuffer(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty);
	[[nodiscard]] AM_Buffer* AllocateMappedBuffer(const uint64_t aSize, const VkBufferUsageFlags aUsage);
	[[nodiscard]] AM_Image* AllocateImage(const VkImageCreateInfo& aCreateInfo, const VkMemoryPropertyFlags aProperty);

	void CopyToMappedMemory(AM_Buffer& aBuffer, void* aSource, const size_t aSize);

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

	// gotta get rid of this
	void AllocateMemory(VkDeviceMemory& outMemoryPtr, const uint32_t aMemoryTypeIndex);

	[[nodiscard]] AM_Buffer* AllocateBufferFast(const uint64_t aSize, const MemoryRequirements& aRequirement);
	[[nodiscard]] AM_Buffer* AllocateBufferSlow(const uint64_t aSize, const MemoryRequirements& aRequirement);
	[[nodiscard]] AM_Buffer* AllocateBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty, MemoryPropertyCache& aCache);

	[[nodiscard]] AM_Buffer* AllocateMappedBufferFast(const uint64_t aSize, const MemoryRequirements& aRequirement);
	[[nodiscard]] AM_Buffer* AllocateMappedBufferSlow(const uint64_t aSize, const MemoryRequirements& aRequirement);
	[[nodiscard]] AM_Buffer* AllocateMappedBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty, MemoryPropertyCache& aCache);

	[[nodiscard]] AM_Image* AllocateImageFast(AM_VkImage& outImage, const VkMemoryRequirements& aRequirement, const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_Image* AllocateImageSlow(AM_VkImage& outImage, const VkMemoryRequirements& aRequirement, const uint32_t aMemoryTypeIndex);
	[[nodiscard]] AM_Image* AllocateImageWithNewBlock(AM_VkImage& outImage, const VkMemoryRequirements& aRequirement, const uint32_t aMemoryTypeIndex);

	uint32_t FindMemoryTypeIndex(const uint32_t someMemoryTypeBits, const VkMemoryPropertyFlags someProperties) const;
	uint64_t GetPaddedSize(const uint64_t aSize, const uint64_t& anAlignmentSize) const;

	std::vector<std::vector<AM_BufferMemoryBlock>> myBufferMemoryPool;
	std::vector<std::vector<AM_ImageMemoryBlock>> myImageMemoryPool;
	std::vector<MemoryPropertyCache> myMemPropCache;
	VkPhysicalDeviceMemoryProperties myPhysicalMemoryProperties;
};

