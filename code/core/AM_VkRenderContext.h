#pragma once

#include "AM_VkSwapChain.h"
#include "AM_Window.h"
#include "TempImage.h"
#include <cassert>

struct VmaAllocationInfo;
VK_DEFINE_HANDLE(VmaAllocator);
class AM_VkRenderContext
{
public:
	explicit AM_VkRenderContext(AM_VkContext& aVkContext, AM_Window& aWindow, VmaAllocator& aVMA);
	~AM_VkRenderContext();

	VkRenderPass GetRenderPass() const { return myRenderPass; }
	bool isFrameInProgress() const { return myIsFrameStarted; }
	VkCommandBuffer GetCurrentCommandBuffer() const
	{
		assert(myIsFrameStarted && "Cannot get command buffer when frame not in progress");
		return myCommandBuffers[myCurrentFrame];
	}

	uint32_t GetFrameIndex() const
	{
		assert(myIsFrameStarted && "Cannot get frame index when frame not in progress");
		return myCurrentFrame;
	}

	VkCommandBuffer BeginFrame();
	void EndFrame();
	void BeginRenderPass(VkCommandBuffer commandBuffer);
	void EndRenderPass(VkCommandBuffer commandBuffer);

	uint32_t GetWidth() const { return mySwapChain.GetWidth(); }
	uint32_t GetHeight() const { return mySwapChain.GetHeight(); }
	float GetAspectRatio() const { return mySwapChain.GetExtentRatio(); }

private:
	AM_VkRenderContext(const AM_VkRenderContext&) = delete;
	AM_VkRenderContext& operator=(const AM_VkRenderContext&) = delete;

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
	void CreateImageView(VkImageView& outImageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels);
	void BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool);
	void EndOneTimeCommands(VkCommandBuffer commandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, VkCommandBuffer aCommandBuffer);
	bool HasStencilComponent(VkFormat format);

	AM_VkContext& myVkContext;
	AM_Window& myWindow;
	VmaAllocator& myVMA;

	AM_VkSwapChain mySwapChain;
	std::vector<VkCommandBuffer> myCommandBuffers;
	std::vector<VkCommandBuffer> myComputeCommandBuffers;

	// for swapchain
	std::vector<VkSemaphore> myImageAvailableSemaphores;
	std::vector<VkSemaphore> myRenderFinishedSemaphores;
	std::vector<VkSemaphore> myComputeFinishedSemaphores;
	std::vector<VkFence> myInFlightFences;
	std::vector<VkFence> myComputeInFlightFences;
	std::vector<VkFramebuffer> myFramebuffers;
	VkRenderPass myRenderPass;
	VkImageView myColorImageView;
	VkImageView myDepthImageView;

	TempImage myColorImage;
	TempImage myDepthImage;

	uint32_t myCurrentFrame;
	uint32_t myImageIndex;
	bool myIsFrameStarted;
};

