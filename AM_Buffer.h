#pragma once
#include "AM_AllocationObject.h"

class AM_Buffer : public AM_AllocationObject
{
public:
	AM_Buffer() 
		: AM_AllocationObject(AM_AllocationObject::BUFFER)
		, myBuffer(VK_NULL_HANDLE)
	{
	}

	AM_Buffer(const VkBuffer aBuffer, const uint64_t anOffset, const uint64_t aSize, const VkDeviceMemory aMemory)
		: AM_AllocationObject(AM_AllocationObject::BUFFER, anOffset, aSize, aMemory)
		, myBuffer(aBuffer)
	{
	}

	~AM_Buffer() = default;

	void Bind(const uint64_t anOffset, const uint64_t aSize, const VkBuffer aBuffer, const VkDeviceMemory aMemory)
	{
		AM_AllocationObject::Bind(anOffset, aSize, aMemory);
		myBuffer = aBuffer;
	}

	void Reset()
	{
		AM_AllocationObject::Reset();
		myBuffer = VK_NULL_HANDLE;
	}
private:
	VkBuffer myBuffer;
};

