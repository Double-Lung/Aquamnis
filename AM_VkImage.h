#pragma once
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkContext.h"

struct AM_VkImage
{
	AM_VkImage()
		: myMemoryObject(nullptr)
		, myImage(nullptr)
	{
	}

	~AM_VkImage()
	{
		Release();
	}

	void Bind(AM_SimpleMemoryObject* aMemoryObject)
	{
		myMemoryObject = aMemoryObject;
		myMemoryObject->myIsEmpty = false;
		vkBindImageMemory(AM_VkContext::device, myImage, myMemoryObject->myMemory, myMemoryObject->myOffset);
	}

	void Init(VkMemoryRequirements& outMemRequirements, const VkImageCreateInfo& someInfo)
	{
		if (vkCreateImage(AM_VkContext::device, &someInfo, nullptr, &myImage) != VK_SUCCESS)
			throw std::runtime_error("failed to create image!");
		vkGetImageMemoryRequirements(AM_VkContext::device, myImage, &outMemRequirements);
	}

	void Release()
	{
		vkDestroyImage(AM_VkContext::device, myImage, nullptr);
		myImage = nullptr;
		myMemoryObject->myIsEmpty = true;
		myMemoryObject = nullptr;
	}

	AM_SimpleMemoryObject* myMemoryObject;
	VkImage myImage;
};

