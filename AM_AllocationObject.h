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
		, myIsEmpty(true)
	{
	}

	AM_AllocationObject(const ObjectType aType)
		: myOffset(0)
		, mySize(0)
		, myMemory(VK_NULL_HANDLE)
		, myType(aType)
		, myIsEmpty(true)
	{
	}

	AM_AllocationObject(const ObjectType aType, const uint64_t anOffset, const uint64_t aSize, const VkDeviceMemory aMemory)
		: myOffset(anOffset)
		, mySize(aSize)
		, myMemory(aMemory)
		, myType(aType)
		, myIsEmpty(true)
	{
	}

	virtual ~AM_AllocationObject() = default;

	uint64_t GetOffset() const { return myOffset; }
	uint64_t GetSize() const { return mySize; }
	VkDeviceMemory GetMemory() const { return myMemory; }
	bool IsEmpty() { return myIsEmpty; }

	void SetOffset(const uint64_t anOffset) { myOffset = anOffset; }
	void SetSize(const uint64_t aSize) { mySize = aSize; }
	void SetIsEmpty(bool aState) { myIsEmpty = aState; }

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
		myIsEmpty = true;
	}

	uint64_t myOffset;
	uint64_t mySize;
	VkDeviceMemory myMemory;
	ObjectType myType;
	bool myIsEmpty;
};

