#pragma once
#include "AM_Image.h"
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkImage.h"

class AM_ImageMemoryBlock : public AM_SimpleMemoryBlock
{
	AM_ImageMemoryBlock() = default;
	~AM_ImageMemoryBlock() = default;

private:
	std::list<AM_Image> myAllocationList;
	AM_VkImage myImage;
};