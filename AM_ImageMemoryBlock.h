#pragma once
#include "AM_Image.h"
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkImage.h"

class AM_ImageMemoryBlock : public AM_SimpleMemoryBlock
{
public:
	AM_ImageMemoryBlock()
		: AM_SimpleMemoryBlock(AM_SimpleMemoryBlock::IMAGE)
	{
	}

	AM_ImageMemoryBlock(AM_ImageMemoryBlock&& aMemoryBlock) noexcept
		: AM_SimpleMemoryBlock(std::move(aMemoryBlock))
	{
		*this = std::move(aMemoryBlock);
	}

	~AM_ImageMemoryBlock()
	{
		myAllocationList.clear();
		if (myMemory)
		{
			vkFreeMemory(AM_VkContext::device, myMemory, nullptr);
			myMemory = nullptr;
		}
	}

	std::list<AM_Image> myAllocationList;

private:
	void* GetImageOrBufferHandle() const override { return nullptr; }

	AM_ImageMemoryBlock(const AM_ImageMemoryBlock& aMemoryBlock) = delete;
	AM_ImageMemoryBlock& operator=(const AM_ImageMemoryBlock& aMemoryBlock) = delete;
	AM_ImageMemoryBlock& operator=(AM_ImageMemoryBlock&& aMemoryBlock) noexcept
	{
		if (this == &aMemoryBlock)
			return *this;

		myAllocationList = std::move(aMemoryBlock.myAllocationList);
		return *this;
	}
};