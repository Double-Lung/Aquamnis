#pragma once
#include "AM_VkPrimitives.h"
#include "AM_VkContext.h"

class AM_VkSwapChain
{
public:
	AM_VkSwapChain()
		: myExtent{0, 0}
		, myChain(nullptr)
	{
	}

	~AM_VkSwapChain()
	{
		Destroy();
	}

	void SetExtent(uint32_t width, uint32_t height)
	{
		myExtent.width = width;
		myExtent.height = height;
	}

	const VkExtent2D& GetExtent() const { return myExtent; }
	uint32_t GetWidth() const { return myExtent.width; }
	uint32_t GetHeight() const { return myExtent.height; }
	float GetExtentRatio() const { return (float)myExtent.width / (float)myExtent.height; }

	void Create(const VkSwapchainCreateInfoKHR& aCreateInfo)
	{
		if (vkCreateSwapchainKHR(AM_VkContext::device, &aCreateInfo, nullptr, &myChain) != VK_SUCCESS)
			throw std::runtime_error("failed to create swap chain!");
		SetSwapChainImages();
	}

	void CreateImageViews(VkImageViewCreateInfo& aCreateInfo)
	{
		myImageViews.resize(myImages.size());
		for (int i = 0; i < myImages.size(); ++i)
		{
			aCreateInfo.image = myImages[i];
			myImageViews[i].CreateView(aCreateInfo);
		}
	}

	void Destroy()
	{
		myImageViews.clear();
		myImages.clear();
		vkDestroySwapchainKHR(AM_VkContext::device, myChain, nullptr);
	}

	VkSwapchainKHR GetSwapChain() { return myChain; }
	const std::vector<VkImage>& GetImages() const { return myImages; }
	const std::vector<AM_VkImageView>& GetImageViews() const { return myImageViews; }

private:

	void SetSwapChainImages()
	{
		uint32_t imageCount;
		vkGetSwapchainImagesKHR(AM_VkContext::device, myChain, &imageCount, nullptr);
		myImages.resize(imageCount);
		vkGetSwapchainImagesKHR(AM_VkContext::device, myChain, &imageCount, myImages.data());
	}

	std::vector<VkImage> myImages;
	std::vector<AM_VkImageView> myImageViews;
	VkExtent2D myExtent;
	VkSwapchainKHR myChain;
};
