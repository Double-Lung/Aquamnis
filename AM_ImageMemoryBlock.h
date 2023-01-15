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

	~AM_ImageMemoryBlock()
	{
		myAllocationList.clear();
		myImage.Release();
		if (myMemory)
			vkFreeMemory(VkDrawContext::device, myMemory, nullptr);
	}

private:
	std::list<AM_Image> myAllocationList;
	AM_VkImage myImage;
};