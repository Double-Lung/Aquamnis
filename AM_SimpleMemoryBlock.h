#pragma once
#include <assert.h>
#include <list>
#include <vulkan/vulkan.h>

struct AM_SimpleMemoryObject
{
	AM_SimpleMemoryObject()
		: myOffset(0)
		, mySize(0)
		, myMemory(nullptr)
		, myIsEmpty(true)
	{
	}

	AM_SimpleMemoryObject(const uint64_t anOffset, const uint64_t aSize, const VkDeviceMemory aMemory)
		: myOffset(anOffset)
		, mySize(aSize)
		, myMemory(aMemory)
		, myIsEmpty(true)
	{
	}

	~AM_SimpleMemoryObject() = default;

	uint64_t myOffset;
	uint64_t mySize;
	VkDeviceMemory myMemory;
	bool myIsEmpty;
};

class AM_SimpleMemoryBlock
{
public:
	AM_SimpleMemoryBlock()
		: myMemory(nullptr)
		, myMappedMemory(nullptr)
		, myExtent(0)
	{
	}

	~AM_SimpleMemoryBlock()
	{ 
		assert(myMemory == nullptr && "VkDeviceMemory not freed!"); 
	}
	AM_SimpleMemoryBlock(const AM_SimpleMemoryBlock&) = delete;
	void operator=(const AM_SimpleMemoryBlock&) = delete;

	AM_SimpleMemoryBlock(AM_SimpleMemoryBlock&& aMemoryBlock) noexcept
		: myMemory(nullptr)
		, myExtent(0)
	{
		*this = std::move(aMemoryBlock);
	}

	std::list<AM_SimpleMemoryObject> myAllocations;
	VkDeviceMemory myMemory;
	void* myMappedMemory;
	uint64_t myExtent;

private:
	AM_SimpleMemoryBlock& operator=(AM_SimpleMemoryBlock&& aMemoryBlock) noexcept
	{
		if (myMemory || this == &aMemoryBlock)
			return *this;

		myMemory = std::exchange(aMemoryBlock.myMemory, nullptr);
		myMappedMemory = std::exchange(aMemoryBlock.myMappedMemory, nullptr);
		myExtent = std::exchange(aMemoryBlock.myExtent, 0);
		return *this;
	}
};
