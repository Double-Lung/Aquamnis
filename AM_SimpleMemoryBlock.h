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
	enum ResourceType
	{
		NOTSET,
		BUFFER,
		IMAGE
	};

	AM_SimpleMemoryBlock(const ResourceType aType)
		: myExtent(0)
		, myAlignment(0)
		, myType(aType)
		, myMemory(VK_NULL_HANDLE)
		, myMappedMemory(nullptr)
		, myIsMapped(false)
	{
	}

	AM_SimpleMemoryBlock(AM_SimpleMemoryBlock&& aMemoryBlock) noexcept
		: myExtent(0)
		, myAlignment(0)
		, myType(NOTSET)
		, myMemory(VK_NULL_HANDLE)
		, myMappedMemory(nullptr)
		, myIsMapped(false)
	{
		*this = std::move(aMemoryBlock);
	}

	virtual ~AM_SimpleMemoryBlock() { assert(!myMemory); }

	std::list<AM_SimpleMemoryObject> myAllocations;
	uint64_t myExtent;
	uint64_t myAlignment;
	ResourceType myType;
	VkDeviceMemory myMemory;
	void* myMappedMemory;
	bool myIsMapped;

private:
	AM_SimpleMemoryBlock() = delete;
	AM_SimpleMemoryBlock(const AM_SimpleMemoryBlock&) = delete;
	void operator=(const AM_SimpleMemoryBlock&) = delete;

	AM_SimpleMemoryBlock& operator=(AM_SimpleMemoryBlock&& aMemoryBlock) noexcept
	{
		if (this == &aMemoryBlock)
			return *this;
		myExtent = std::exchange(aMemoryBlock.myExtent, 0);
		myAlignment = std::exchange(aMemoryBlock.myAlignment, 0);
		myType = std::exchange(aMemoryBlock.myType, NOTSET);
		myMemory = std::exchange(aMemoryBlock.myMemory, nullptr);
		myMappedMemory = std::exchange(aMemoryBlock.myMappedMemory, nullptr);
		myIsMapped = std::exchange(aMemoryBlock.myIsMapped, false);
		return *this;
	}
};
