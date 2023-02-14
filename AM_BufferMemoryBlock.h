#pragma once
#include "AM_Buffer.h"
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkBuffer.h"

class AM_BufferMemoryBlock : public AM_SimpleMemoryBlock
{
public:
	AM_BufferMemoryBlock()
		: AM_SimpleMemoryBlock(AM_SimpleMemoryBlock::BUFFER)
		, myFreeSlotCount(0) // move to parent?
	{
	}

	AM_BufferMemoryBlock(const uint64_t anExtent, ResourceType aType, const VkDeviceMemory aMemory, AM_VkBuffer&& aBuffer)
		: AM_SimpleMemoryBlock(anExtent, aType, aMemory)
		, myBuffer(std::move(aBuffer))
		, myFreeSlotCount(0)
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

	void SetBuffer(AM_VkBuffer& aVkBuffer) { myBuffer = std::move(aVkBuffer); }

	void BindBufferMemory(const uint32_t aMemoryTypeIndex)
	{
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = VkDrawConstants::SINGLEALLOCSIZE;
		allocInfo.memoryTypeIndex = aMemoryTypeIndex;
		if (vkAllocateMemory(VkDrawContext::device, &allocInfo, nullptr, &myMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate memory of type ??? !");

		vkBindBufferMemory(VkDrawContext::device, myBuffer.myBuffer, myMemory, 0);
	}

	AM_Buffer* Allocate(const uint64_t aSize)
	{
		AM_Buffer& buffer = myAllocationList.emplace_back(myBuffer.myBuffer, myExtent, aSize, myMemory);
		myExtent += aSize;
		return &buffer;
	}

	bool HasFreeSlot() { return myFreeSlotCount > 0; }

	std::list<AM_Buffer> myAllocationList;
	AM_VkBuffer myBuffer;
	uint32_t myFreeSlotCount;

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
		myFreeSlotCount = std::exchange(aMemoryBlock.myFreeSlotCount, 0);
		return *this;
	}
};