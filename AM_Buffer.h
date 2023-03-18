#pragma once
#include "AM_AllocationObject.h"

class AM_Buffer : public AM_AllocationObject
{
public:
	AM_Buffer(const void* aBuffer, const uint64_t anOffset, const uint64_t aSize)
		: AM_AllocationObject(AM_AllocationObject::BUFFER, anOffset, aSize)
		, myBuffer((VkBuffer)aBuffer)
	{
	}

	AM_Buffer(AM_Buffer&& aBuffer) noexcept
		: AM_AllocationObject(std::move(aBuffer))
	{
		*this = std::move(aBuffer);
	}

	~AM_Buffer() = default;

	VkBuffer myBuffer;
private:
	AM_Buffer& operator=(AM_Buffer&& aBuffer) noexcept
	{
		if (this == &aBuffer)
			return *this;

		myBuffer = std::exchange(aBuffer.myBuffer, nullptr);
		return *this;
	}
	AM_Buffer() = delete;
	AM_Buffer(const AM_Buffer& aBuffer) = delete;
	AM_Buffer& operator=(const AM_Buffer& aBuffer) = delete;
};

