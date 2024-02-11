#pragma once
#include "AM_VkContext.h"

class AM_VkSwapChain
{
public:
	AM_VkSwapChain(AM_VkContext& aVkContext);
	~AM_VkSwapChain() { Destroy(); }

	void SetExtent(uint32_t width, uint32_t height);
	const VkExtent2D& GetExtent() const { return myExtent; }
	uint32_t GetWidth() const { return myExtent.width; }
	uint32_t GetHeight() const { return myExtent.height; }
	float GetExtentRatio() const { return (float)myExtent.width / (float)myExtent.height; }
	VkSwapchainKHR GetSwapChain() { return myChain; }
	const std::vector<VkImage>& GetImages() const { return myImages; }
	const std::vector<VkImageView>& GetImageViews() const { return myImageViews; }

	void Create(const VkSwapchainCreateInfoKHR& aCreateInfo);
	void CreateImageViews(VkImageViewCreateInfo& aCreateInfo);
	void Destroy();

private:
	void SetSwapChainImages();

	std::vector<VkImage> myImages;
	std::vector<VkImageView> myImageViews;
	VkExtent2D myExtent;
	VkSwapchainKHR myChain;
	AM_VkContext& myVkContext;
};
