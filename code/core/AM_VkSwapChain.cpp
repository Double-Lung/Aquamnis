#include "AM_VkSwapChain.h"

AM_VkSwapChain::AM_VkSwapChain(AM_VkContext& aVkContext) 
	: myVkContext(aVkContext)
	, myExtent{ 0, 0 }
	, myChain(nullptr)
{
}

void AM_VkSwapChain::SetExtent(uint32_t width, uint32_t height)
{
	myExtent.width = width;
	myExtent.height = height;
}

void AM_VkSwapChain::Create(const VkSwapchainCreateInfoKHR& aCreateInfo)
{
	if (vkCreateSwapchainKHR(myVkContext.device, &aCreateInfo, nullptr, &myChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain!");
	SetSwapChainImages();
}

void AM_VkSwapChain::CreateImageViews(VkImageViewCreateInfo& aCreateInfo)
{
	myImageViews.resize(myImages.size());
	for (int i = 0; i < myImages.size(); ++i)
	{
		aCreateInfo.image = myImages[i];
		myImageViews[i] = myVkContext.CreateImageView(aCreateInfo);
	}
}

void AM_VkSwapChain::Destroy()
{
	for (VkImageView imageView : myImageViews)
		myVkContext.DestroyImageView(imageView);
	myImageViews.clear();
	myImages.clear();
	vkDestroySwapchainKHR(myVkContext.device, myChain, nullptr);
}

void AM_VkSwapChain::SetSwapChainImages()
{
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(myVkContext.device, myChain, &imageCount, nullptr);
	myImages.resize(imageCount);
	vkGetSwapchainImagesKHR(myVkContext.device, myChain, &imageCount, myImages.data());
}
