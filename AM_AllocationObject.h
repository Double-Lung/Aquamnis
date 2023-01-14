#pragma once
#include <vulkan/vulkan.h>

class AM_AllocationObject
{
public:
	enum ObjectType
	{
		NOTSET,
		BUFFER,
		IMAGE,
	};

	AM_AllocationObject()
		: myOffset(0)
		, mySize(0)
		, myMemory(VK_NULL_HANDLE)
		, myType(NOTSET)
	{
	}

	AM_AllocationObject(const ObjectType aType)
		: myOffset(0)
		, mySize(0)
		, myMemory(VK_NULL_HANDLE)
		, myType(aType)
	{
	}

	AM_AllocationObject(const ObjectType aType, const uint64_t anOffset, const uint64_t aSize, const VkDeviceMemory aMemory)
		: myOffset(anOffset)
		, mySize(aSize)
		, myMemory(aMemory)
		, myType(aType)
	{
	}

	virtual ~AM_AllocationObject() = default;

	uint64_t GetOffset() const { return myOffset; }
	uint64_t GetSize() const { return mySize; }
	VkDeviceMemory GetMemory() const { return myMemory; }

protected:
	void Bind(const uint64_t anOffset, const uint64_t aSize, const VkDeviceMemory aMemory)
	{
		myOffset = anOffset;
		mySize = aSize;
		myMemory = aMemory;
	}

	void Reset()
	{
		myOffset = 0;
		mySize = 0;
		myMemory = VK_NULL_HANDLE;
	}

	uint64_t myOffset;
	uint64_t mySize;
	VkDeviceMemory myMemory;
	ObjectType myType;
};

