#pragma once
#include "AM_Buffer.h"
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkBuffer.h"

class AM_BufferMemoryBlock : public AM_SimpleMemoryBlock
{
	AM_BufferMemoryBlock() = default;
	~AM_BufferMemoryBlock() = default;

private:
	std::list<AM_Buffer> myAllocationList;
	AM_VkBuffer myBuffer;
};