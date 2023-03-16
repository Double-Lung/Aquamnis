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

	AM_BufferMemoryBlock(AM_BufferMemoryBlock&& aMemoryBlock) noexcept
		: AM_SimpleMemoryBlock(std::move(aMemoryBlock))
	{
		*this = std::move(aMemoryBlock);
	}

	~AM_BufferMemoryBlock()
	{
		myAllocationList.clear();
		myBuffer.Release();
		if (myMemory)
		{
			vkFreeMemory(AM_VkContext::device, myMemory, nullptr);
			myMemory = nullptr;
		}	
	}

	void Init(const uint32_t aMemoryTypeIndex, const uint64_t anAlignment)
	{
		myAlignment = anAlignment;
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = AM_VkRenderCoreConstants::SINGLEALLOCSIZE;
		allocInfo.memoryTypeIndex = aMemoryTypeIndex;
		if (vkAllocateMemory(AM_VkContext::device, &allocInfo, nullptr, &myMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate memory of type ??? !");
	}

	AM_Buffer* Allocate(const uint64_t aSize)
	{
		AM_Buffer& buffer = myAllocationList.emplace_back(myBuffer.myBuffer, myExtent, aSize);
		buffer.SetMemoryHandle(myMemory);
		buffer.SetIsEmpty(false);
		myExtent += aSize;
		return &buffer;
	}

	AM_Buffer* AllocateSlow(const uint64_t aSize)
	{
		for (auto slotIter = myAllocationList.begin(); slotIter != myAllocationList.end(); ++slotIter)
		{
			if (!(slotIter->IsEmpty() && slotIter->GetSize() >= aSize))
				continue;

			const uint64_t leftover = slotIter->GetSize() - aSize;
			if (leftover == 0)
			{
				slotIter->SetIsEmpty(false);
				return &(*slotIter);
			}

			myAllocationList.emplace(slotIter, myBuffer.myBuffer, slotIter->GetOffset(), leftover);
			slotIter->SetOffset(slotIter->GetOffset() + leftover);
			slotIter->SetMemoryHandle(myMemory);
			slotIter->SetSize(aSize);
			slotIter->SetIsEmpty(false);
			return &(*slotIter);
		}

		return nullptr;
	}

	std::list<AM_Buffer> myAllocationList;
	AM_VkBuffer myBuffer;

private:
	AM_BufferMemoryBlock(const AM_BufferMemoryBlock& aMemoryBlock) = delete;
	AM_BufferMemoryBlock& operator=(const AM_BufferMemoryBlock& aMemoryBlock) = delete;
	AM_BufferMemoryBlock& operator=(AM_BufferMemoryBlock&& aMemoryBlock) noexcept
	{
		if (this == &aMemoryBlock)
			return *this;

		myAllocationList = std::move(aMemoryBlock.myAllocationList);
		myBuffer = std::move(aMemoryBlock.myBuffer);
		return *this;
	}
};