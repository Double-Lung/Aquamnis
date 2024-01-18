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
	inline VkCommandBuffer GetCurrentCommandBuffer() const
	{
		assert(myIsFrameStarted && "Cannot get command buffer when frame not in progress");
		return myCommandBuffers[myCurrentFrame];
	}

	inline VkCommandBuffer GetCurrentComputeCommandBuffer() const
	{
		assert(myIsFrameStarted && "Cannot get command buffer when frame not in progress");
		return myComputeCommandBuffers[myCurrentFrame];
	}

	inline uint32_t GetFrameIndex() const
	{
		assert(myIsFrameStarted && "Cannot get frame index when frame not in progress");
		return myCurrentFrame;
	}

	VkCommandBuffer BeginFrame();
	void EndFrame();
	void BeginRenderPass(VkCommandBuffer commandBuffer);
	void EndRenderPass(VkCommandBuffer commandBuffer);
	void SubmitComputeQueue();

	uint32_t GetWidth() const { return mySwapChain.GetWidth(); }
	uint32_t GetHeight() const { return mySwapChain.GetHeight(); }
	float GetAspectRatio() const { return mySwapChain.GetExtentRatio(); }

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
	void CreateSyncObjects();

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
	std::vector<VkCommandBuffer> myComputeCommandBuffers;

	// for swapchain
	std::vector<AM_VkSemaphore> myImageAvailableSemaphores;
	std::vector<AM_VkSemaphore> myRenderFinishedSemaphores;
	std::vector<AM_VkSemaphore> myComputeFinishedSemaphores;
	std::vector<AM_VkFence> myInFlightFences;
	std::vector<AM_VkFence> myComputeInFlightFences;
	std::vector<AM_VkFramebuffer> myFramebuffers;
	AM_VkRenderPass myRenderPass;
	AM_VkImageView myColorImageView;
	AM_VkImageView myDepthImageView;
	AM_Image* myColorImage;
	AM_Image* myDepthImage;

	uint32_t myCurrentFrame;
	uint32_t myImageIndex;
	bool myIsFrameStarted;
};

