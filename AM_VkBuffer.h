#pragma once
#include "AM_SimpleMemoryBlock.h"
#include "VkDrawContext.h"

struct AM_VkBuffer
{
	AM_VkBuffer()
		: myMemoryObject(nullptr)
		, myBuffer(nullptr)
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

	void Init(VkMemoryRequirements& outMemRequirements, const VkBufferCreateInfo& someInfo)
	{
		if (vkCreateBuffer(VkDrawContext::device, &someInfo, nullptr, &myBuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");
		vkGetBufferMemoryRequirements(VkDrawContext::device, myBuffer, &outMemRequirements);
	}

	void Release()
	{
		vkDestroyBuffer(VkDrawContext::device, myBuffer, nullptr);
		myBuffer = nullptr;
		myMemoryObject->myIsEmpty = true;
		myMemoryObject = nullptr;
	}

	AM_SimpleMemoryObject* myMemoryObject;
	VkBuffer myBuffer;
};

