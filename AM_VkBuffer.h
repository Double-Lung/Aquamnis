#pragma once
#include "AM_SimpleMemoryBlock.h"
#include "VkDrawContext.h"
#include <assert.h>

enum class AM_BufferTypeBits : uint8_t
{
	UNSET = 0x0,
	DEVICEONLY = 0x1,
	UNIFORM = 0x2,
	STAGING = 0x4
};

struct AM_VkBuffer
{
	AM_VkBuffer()
		: myMemoryObject(nullptr)
		, myBuffer(nullptr)
		, myType(static_cast<uint8_t>(AM_BufferTypeBits::UNSET))
	{
	}

	AM_VkBuffer(AM_VkBuffer&& aBuffer) noexcept
		: myMemoryObject(nullptr)
		, myBuffer(nullptr)
		, myType(static_cast<uint8_t>(AM_BufferTypeBits::UNSET))
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
		myMemoryObject = std::exchange(aBuffer.myMemoryObject, nullptr);
		myBuffer = std::exchange(aBuffer.myBuffer, nullptr);
		myType = std::exchange(aBuffer.myType, 0U);
		return *this;
	}

	void Bind(AM_SimpleMemoryObject* aMemoryObject)
	{
		myMemoryObject = aMemoryObject;
		myMemoryObject->myIsEmpty = false;
		vkBindBufferMemory(VkDrawContext::device, myBuffer, myMemoryObject->myMemory, myMemoryObject->myOffset);
	}

	void Init(VkMemoryRequirements& outMemRequirements, const VkBufferCreateInfo& someInfo, const AM_BufferTypeBits aBufferType)
	{
		if (vkCreateBuffer(VkDrawContext::device, &someInfo, nullptr, &myBuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");
		vkGetBufferMemoryRequirements(VkDrawContext::device, myBuffer, &outMemRequirements);
		myType = static_cast<uint8_t>(aBufferType);
	}

	void Init(const VkBufferCreateInfo& someInfo)
	{
		if (vkCreateBuffer(VkDrawContext::device, &someInfo, nullptr, &myBuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");
	}

	void Init()
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = VkDrawConstants::SINGLEALLOCSIZE;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT
			| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			| VK_BUFFER_USAGE_INDEX_BUFFER_BIT
			| VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(VkDrawContext::device, &bufferInfo, nullptr, &myBuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");

		VkMemoryRequirements dummy;
		vkGetBufferMemoryRequirements(VkDrawContext::device, myBuffer, &dummy);
	}

	void Release()
	{
		if (myBuffer)
		{
			vkDestroyBuffer(VkDrawContext::device, myBuffer, nullptr);
			myBuffer = nullptr;
		}
		myMemoryObject->myIsEmpty = true;
		myMemoryObject = nullptr;
	}

	void CopyToMappedMemory(void* aDestination, void* aSource, const size_t aSize)
	{
		assert(aSize<= myMemoryObject->mySize && "Trying to map more memory than allocated!");
		memcpy((void*)((char*)aDestination + myMemoryObject->myOffset), aSource, aSize);
	} 

	AM_SimpleMemoryObject* myMemoryObject;
	VkBuffer myBuffer;
	uint8_t myType;

private:
	AM_VkBuffer(const AM_VkBuffer& aBuffer) = delete;
	AM_VkBuffer& operator=(const AM_VkBuffer& aBuffer) = delete;
};

