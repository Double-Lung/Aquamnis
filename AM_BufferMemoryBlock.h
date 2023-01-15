#pragma once
#include "AM_Buffer.h"
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkBuffer.h"

class AM_BufferMemoryBlock : public AM_SimpleMemoryBlock
{
public:
	AM_BufferMemoryBlock()
		: AM_SimpleMemoryBlock(AM_SimpleMemoryBlock::BUFFER)
	{
	}

	AM_BufferMemoryBlock(const uint64_t anExtent, ResourceType aType, const VkDeviceMemory aMemory, AM_VkBuffer&& aBuffer)
		: AM_SimpleMemoryBlock(anExtent, aType, aMemory)
		, myBuffer(std::move(aBuffer))
	{
	}

	AM_BufferMemoryBlock(AM_BufferMemoryBlock&& aMemoryBlock) noexcept
		: AM_SimpleMemoryBlock(AM_SimpleMemoryBlock::BUFFER)
	{
		*this = std::move(aMemoryBlock);
	}

	~AM_BufferMemoryBlock()
	{
		myAllocationList.clear();
		myBuffer.Release();
		if (myMemory)
			vkFreeMemory(VkDrawContext::device, myMemory, nullptr);
	}

	void Init(const uint32_t aMemoryTypeIndex)
	{
		myBuffer.Init();

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = VkDrawConstants::SINGLEALLOCSIZE;
		allocInfo.memoryTypeIndex = aMemoryTypeIndex;
		if (vkAllocateMemory(VkDrawContext::device, &allocInfo, nullptr, &myMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate memory of type ??? !");
	}

private:
	AM_BufferMemoryBlock(const AM_BufferMemoryBlock& aMemoryBlock) = delete;
	AM_BufferMemoryBlock& operator=(const AM_BufferMemoryBlock& aMemoryBlock) = delete;
	AM_BufferMemoryBlock& operator=(AM_BufferMemoryBlock&& aMemoryBlock) noexcept
	{
		if (this == &aMemoryBlock)
			return *this;
		myExtent = std::exchange(aMemoryBlock.myExtent, 0);
		myType = std::exchange(aMemoryBlock.myType, NOTSET);
		myMemory = std::exchange(aMemoryBlock.myMemory, nullptr);
		myMappedMemory = std::exchange(aMemoryBlock.myMappedMemory, nullptr);
		myIsMapped = std::exchange(aMemoryBlock.myIsMapped, false);
		myAllocationList = std::move(aMemoryBlock.myAllocationList);
		myBuffer = std::move(aMemoryBlock.myBuffer);
		return *this;
	}

	std::list<AM_Buffer> myAllocationList;
	AM_VkBuffer myBuffer;
};