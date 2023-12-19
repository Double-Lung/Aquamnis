#pragma once
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkContext.h"

struct AM_VkImage
{
	AM_VkImage()
		: myImage(VK_NULL_HANDLE)
	{
	}

	AM_VkImage(const VkImageCreateInfo& someInfo)
		: myImage(VK_NULL_HANDLE)
	{
		if (vkCreateImage(AM_VkContext::device, &someInfo, nullptr, &myImage) != VK_SUCCESS)
			throw std::runtime_error("failed to create image!");
	}

	AM_VkImage(AM_VkImage&& anImage) noexcept
		: myImage(nullptr)
	{
		*this = std::move(anImage);
	}

	~AM_VkImage()
	{
		Release();
	}

	AM_VkImage& operator=(AM_VkImage&& anImage) noexcept
	{
		if (this == &anImage)
			return *this;
		myImage = std::exchange(anImage.myImage, nullptr);
		return *this;
	}

	void Release()
	{
		if (myImage)
		{
			vkDestroyImage(AM_VkContext::device, myImage, nullptr);
			myImage = nullptr;
		}
	}

	VkImage GetImage() const { return myImage; }
	void SetImage(VkImage anImage) { myImage = anImage; }

private:
	VkImage myImage;
	AM_VkImage(const AM_VkImage& anImage) = delete;
	AM_VkImage& operator=(const AM_VkImage& anImage) = delete;
};

