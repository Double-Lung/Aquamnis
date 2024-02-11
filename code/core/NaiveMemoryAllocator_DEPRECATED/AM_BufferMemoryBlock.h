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
			vkFreeMemory(myVkContext.device, myMemory, nullptr);
			myMemory = nullptr;
		}	
	}

	std::list<AM_Buffer> myAllocationList;
	AM_VkBuffer myBuffer;

private:
	void* GetImageOrBufferHandle() const override { return (void*)myBuffer.myBuffer; }

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