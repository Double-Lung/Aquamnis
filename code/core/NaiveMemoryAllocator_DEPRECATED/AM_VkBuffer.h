#pragma once
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkContext.h"
#include <assert.h>

struct AM_VkBuffer
{
	AM_VkBuffer()
		: myBuffer(nullptr)
	{
	}

	AM_VkBuffer(const VkBufferCreateInfo& someInfo)
		: myBuffer(nullptr)
	{
		if (vkCreateBuffer(myVkContext.device, &someInfo, nullptr, &myBuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");
	}

	AM_VkBuffer(AM_VkBuffer&& aBuffer) noexcept
		: myBuffer(nullptr)
	{
		*this = std::move(aBuffer);
	}

	~AM_VkBuffer()
	{
		Release();
	}

	AM_VkBuffer& operator=(AM_VkBuffer&& aBuffer) noexcept
	{
		if (this == &aBuffer)
			return *this;
		myBuffer = std::exchange(aBuffer.myBuffer, nullptr);
		return *this;
	}

	void Release()
	{
		if (myBuffer)
		{
			vkDestroyBuffer(myVkContext.device, myBuffer, nullptr);
			myBuffer = nullptr;
		}
	}

	VkBuffer myBuffer;

private:
	AM_VkBuffer(const AM_VkBuffer& aBuffer) = delete;
	AM_VkBuffer& operator=(const AM_VkBuffer& aBuffer) = delete;
};

