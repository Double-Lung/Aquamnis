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

	AM_Buffer(AM_Buffer&& aBuffer) noexcept
	{
		*this = std::move(aBuffer);
	}

	~AM_Buffer() = default;

	AM_Buffer& operator=(AM_Buffer&& aBuffer) noexcept
	{
		if (this == &aBuffer)
			return *this;
		myOffset = std::exchange(aBuffer.myOffset, 0);
		mySize = std::exchange(aBuffer.mySize, 0);
		myMemory = std::exchange(aBuffer.myMemory, nullptr);
		myType = std::exchange(aBuffer.myType, AM_AllocationObject::NOTSET);
		myBuffer = std::exchange(aBuffer.myBuffer, nullptr);
		return *this;
	}

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
	AM_Buffer(const AM_Buffer& aBuffer) = delete;
	AM_Buffer& operator=(const AM_Buffer& aBuffer) = delete;
	VkBuffer myBuffer;
};

