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

	template <typename T, typename M>
	[[nodiscard]] T* AllocateFast(const uint64_t aSize, const MemoryRequirements& aRequirement, std::vector<std::vector<M>>& aMemoryPool, bool aShouldMap = false);

	template <typename T, typename M>
	[[nodiscard]] T* AllocateSlow(const uint64_t aSize, const MemoryRequirements& aRequirement, std::vector<std::vector<M>>& aMemoryPool, bool aShouldMap = false);

	template <typename T, typename M>
	[[nodiscard]] T* AllocateWithNewBlock(const uint64_t aSize, const MemoryRequirements& aRequirement, std::vector<std::vector<M>>& aMemoryPool, bool aShouldMap = false);

	uint32_t FindMemoryTypeIndex(const uint32_t someMemoryTypeBits, const VkMemoryPropertyFlags someProperties) const;
	uint64_t GetPaddedSize(const uint64_t aSize, const uint64_t& anAlignmentSize) const;

	std::vector<std::vector<AM_BufferMemoryBlock>> myBufferMemoryPool;
	std::vector<std::vector<AM_ImageMemoryBlock>> myImageMemoryPool;
	std::vector<MemoryPropertyCache> myMemPropCache;
	VkPhysicalDeviceMemoryProperties myPhysicalMemoryProperties;
};

template <typename T, typename M>
[[nodiscard]] T* AM_NaiveMemoryAllocator::AllocateFast(const uint64_t aSize, const MemoryRequirements& aRequirement, std::vector<std::vector<M>>& aMemoryPool, bool aShouldMap)
{
	std::vector<M>& memArray = aMemoryPool[aRequirement.myMemoryTypeIndex];
	for (M& block : memArray)
	{
		if (block.myAlignment != aRequirement.myAlignment)
			continue;

		const uint64_t paddedSize = GetPaddedSize(aSize, block.myAlignment);
		if (block.myExtent + paddedSize > AM_VkRenderCoreConstants::SINGLEALLOCSIZE)
			continue;

		T* obj = block.Allocate(paddedSize, block.myAllocationList);
		if (aShouldMap)
		{
			if (!block.myIsMapped)
			{
				vkMapMemory(AM_VkContext::device, block.myMemory, 0, AM_VkRenderCoreConstants::SINGLEALLOCSIZE, 0, &block.myMappedMemory);
				block.myIsMapped = true;
			}
			obj->SetMappedMemory(block.myMappedMemory);
		}
		
		return obj;
	}
	return nullptr;
}

template <typename T, typename M>
[[nodiscard]] T* AM_NaiveMemoryAllocator::AllocateSlow(const uint64_t aSize, const MemoryRequirements& aRequirement, std::vector<std::vector<M>>& aMemoryPool, bool aShouldMap)
{
	std::vector<M>& memArray = aMemoryPool[aRequirement.myMemoryTypeIndex];
	for (M& block : memArray)
	{
		if (block.myAlignment != aRequirement.myAlignment)
			continue;

		T* obj = block.AllocateSlow(GetPaddedSize(aSize, block.myAlignment), block.myAllocationList);
		if (!obj)
			continue;

		if (aShouldMap)
		{
			if (!block.myIsMapped)
			{
				vkMapMemory(AM_VkContext::device, block.myMemory, 0, AM_VkRenderCoreConstants::SINGLEALLOCSIZE, 0, &block.myMappedMemory);
				block.myIsMapped = true;
			}
			obj->SetMappedMemory(block.myMappedMemory);
		}
		
		return obj;
	}

	return nullptr;
}

template <typename T, typename M>
[[nodiscard]] T* AM_NaiveMemoryAllocator::AllocateWithNewBlock(const uint64_t aSize, const MemoryRequirements& aRequirement, std::vector<std::vector<M>>& aMemoryPool, bool aShouldMap)
{
	std::vector<M>& memArray = aMemoryPool[aRequirement.myMemoryTypeIndex];
	M& newBlock = memArray.emplace_back();
	newBlock.Init(aRequirement.myMemoryTypeIndex, aRequirement.myAlignment);

	T* obj = newBlock.Allocate(GetPaddedSize(aSize, aRequirement.myAlignment), newBlock.myAllocationList);

	if (aShouldMap)
	{
		vkMapMemory(AM_VkContext::device, newBlock.myMemory, 0, AM_VkRenderCoreConstants::SINGLEALLOCSIZE, 0, &newBlock.myMappedMemory);
		newBlock.myIsMapped = true;
		obj->SetMappedMemory(newBlock.myMappedMemory);
	}

	return obj;
}

