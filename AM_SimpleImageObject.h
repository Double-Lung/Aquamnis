#pragma once
#include "AM_SimpleMemoryBlock.h"
#include "VkDrawContext.h"
#include <stdexcept>

struct AM_SimpleImageObject
{
	AM_SimpleImageObject()
		: myMemoryObject(nullptr)
		, myImage(nullptr)
	{
	}

	~AM_SimpleImageObject()
	{
		Release();
	}

	void Bind(AM_SimpleMemoryObject* aMemoryObject)
	{
		myMemoryObject = aMemoryObject;
		myMemoryObject->myIsEmpty = false;
		vkBindImageMemory(VkDrawContext::device, myImage, myMemoryObject->myMemory, myMemoryObject->myOffset);
	}

	void Init(VkMemoryRequirements& outMemRequirements, const VkImageCreateInfo& someInfo)
	{
		if (vkCreateImage(VkDrawContext::device, &someInfo, nullptr, &myImage) != VK_SUCCESS)
			throw std::runtime_error("failed to create image!");
		vkGetImageMemoryRequirements(VkDrawContext::device, myImage, &outMemRequirements);
	}

	void Release()
	{
		vkDestroyImage(VkDrawContext::device, myImage, nullptr);
		myMemoryObject->myIsEmpty = true;
		myMemoryObject = nullptr;
	}

	AM_SimpleMemoryObject* myMemoryObject;
	VkImage myImage;
};

