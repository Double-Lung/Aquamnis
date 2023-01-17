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

	AM_ImageMemoryBlock(AM_ImageMemoryBlock&& anImageMemoryBlock) noexcept
	{

	}

	~AM_ImageMemoryBlock()
	{
		myAllocationList.clear();
		myImage.Release();
		if (myMemory)
			vkFreeMemory(VkDrawContext::device, myMemory, nullptr);
	}

	

private:
	AM_ImageMemoryBlock(const AM_ImageMemoryBlock& aMemoryBlock) = delete;
	AM_ImageMemoryBlock& operator=(const AM_ImageMemoryBlock& aMemoryBlock) = delete;
	AM_ImageMemoryBlock& operator=(AM_ImageMemoryBlock&& aMemoryBlock) noexcept
	{
		return *this;
	}

	std::list<AM_Image> myAllocationList;
	AM_VkImage myImage;
};