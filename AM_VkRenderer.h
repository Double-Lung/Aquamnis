#pragma once

#include "AM_NaiveMemoryAllocator.h"
#include "AM_VkSwapChain.h"
#include "AM_Window.h"

class AM_Image;
class AM_VkRenderer
{
public:
	explicit AM_VkRenderer(AM_VkContext& aVkContext, AM_Window& aWindow, AM_NaiveMemoryAllocator& aMemoryAllocator);
	~AM_VkRenderer();

	VkRenderPass GetRenderPass() const { return myRenderPass.myPass; }
	bool isFrameInProgress() const { return myIsFrameStarted; }
	inline VkCommandBuffer GetCurrentCommandBuffer() const;
	inline uint32_t GetFrameIndex() const;

	VkCommandBuffer BeginFrame();
	void EndFrame();
	void BeginRenderPass(VkCommandBuffer commandBuffer);
	void EndRenderPass(VkCommandBuffer commandBuffer);

private:
	AM_VkRenderer(const AM_VkRenderer&) = delete;
	AM_VkRenderer& operator=(const AM_VkRenderer&) = delete;

	void CreateReusableCommandBuffers();
	void FreeCommandBuffers();
	void RecreateSwapChain();

	// these should go into swapchain
	void CreateRenderPass();
	void CreateSwapChain();
	void CleanupSwapChain();
	void CreateColorResources();
	void CreateDepthResources();
	void CreateFramebuffers();

	// temp utils
	AM_Image* CreateImage(const VkExtent2D& anExtent, uint32_t aMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void CreateImageView(AM_VkImageView& outImageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels);
	void BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool);
	void EndOneTimeCommands(VkCommandBuffer commandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, VkCommandBuffer aCommandBuffer);
	bool HasStencilComponent(VkFormat format);

	AM_VkContext& myVkContext;
	AM_Window& myWindow;
	AM_NaiveMemoryAllocator& myMemoryAllocator;

	AM_VkSwapChain mySwapChain;
	std::vector<VkCommandBuffer> myCommandBuffers;

	// for swapchain
	std::vector<AM_VkFramebuffer> myFramebuffers;
	AM_VkRenderPass myRenderPass;
	AM_VkImageView myColorImageView;
	AM_VkImageView myDepthImageView;
	AM_Image* myColorImage = nullptr;
	AM_Image* myDepthImage = nullptr;

	uint32_t myCurrentFrame;
	uint32_t myImageIndex;
	bool myIsFrameStarted;
};
