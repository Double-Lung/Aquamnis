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

	~AM_VkBuffer()
	{
		Release();
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

	void Release()
	{
		vkDestroyBuffer(VkDrawContext::device, myBuffer, nullptr);
		myBuffer = nullptr;
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
};

