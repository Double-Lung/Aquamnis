#pragma once
#include "AM_VkContext.h"

struct AM_VkSemaphore
{
	AM_VkSemaphore()
		: mySemaphore(nullptr)
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(AM_VkContext::device, &semaphoreInfo, nullptr, &mySemaphore) != VK_SUCCESS)
			throw std::runtime_error("failed to create semaphores!");
	}
	~AM_VkSemaphore()
	{
		vkDestroySemaphore(AM_VkContext::device, mySemaphore, nullptr);
	}

	VkSemaphore mySemaphore;
};

struct AM_VkFence
{
	AM_VkFence()
		: myFence(nullptr)
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(AM_VkContext::device, &fenceInfo, nullptr, &myFence) != VK_SUCCESS)
			throw std::runtime_error("failed to create semaphores!");
	}

	~AM_VkFence()
	{
		vkDestroyFence(AM_VkContext::device, myFence, nullptr);
	}

	VkFence myFence;
};

struct AM_VkPipelineLayout
{
	AM_VkPipelineLayout()
		: myLayout(nullptr)
	{
	}

	~AM_VkPipelineLayout()
	{
		vkDestroyPipelineLayout(AM_VkContext::device, myLayout, nullptr);
	}

	void CreateLayout(const VkPipelineLayoutCreateInfo& aCreateInfo)
	{
		if (vkCreatePipelineLayout(AM_VkContext::device, &aCreateInfo, nullptr, &myLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create pipeline layout!");
	}

	VkPipelineLayout myLayout;
};

struct AM_VkPipeline
{
	AM_VkPipeline()
		: myPipeline(nullptr)
	{
	}

	~AM_VkPipeline()
	{
		vkDestroyPipeline(AM_VkContext::device, myPipeline, nullptr);
	}

	void CreatePipeline(const VkGraphicsPipelineCreateInfo& aCreateInfo)
	{
		if (vkCreateGraphicsPipelines(AM_VkContext::device, VK_NULL_HANDLE, 1, &aCreateInfo, nullptr, &myPipeline) != VK_SUCCESS)
			throw std::runtime_error("failed to create graphics pipeline!");
	}

	VkPipeline myPipeline;
};

struct AM_VkFramebuffer
{
	AM_VkFramebuffer()
		: myFramebuffer(nullptr)
	{
	}

	~AM_VkFramebuffer()
	{
		DestroyFrameBuffer();
	}

	void CreateFrameBuffer(const VkFramebufferCreateInfo& aCreateInfo)
	{
		if (vkCreateFramebuffer(AM_VkContext::device, &aCreateInfo, nullptr, &myFramebuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}

	void DestroyFrameBuffer()
	{
		vkDestroyFramebuffer(AM_VkContext::device, myFramebuffer, nullptr);
	}

	VkFramebuffer myFramebuffer;
};

struct AM_VkCommandBuffer
{
	AM_VkCommandBuffer()
		: myCommandBuffer(nullptr)
	{
	}

	~AM_VkCommandBuffer()
	{
	}

	void CreateCommandBuffer(const VkCommandBufferAllocateInfo& aCreateInfo)
	{
		if (vkAllocateCommandBuffers(AM_VkContext::device, &aCreateInfo, &myCommandBuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate command buffer!");
	}

	VkCommandBuffer myCommandBuffer;
};

struct AM_VkSampler
{
	AM_VkSampler()
		: mySampler(nullptr)
	{
	}

	~AM_VkSampler()
	{
		vkDestroySampler(AM_VkContext::device, mySampler, nullptr);
	}

	void CreateSampler(const VkSamplerCreateInfo& aCreateInfo)
	{
		if (vkCreateSampler(AM_VkContext::device, &aCreateInfo, nullptr, &mySampler) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture sampler!");
	}

	VkSampler mySampler;
};

struct AM_VkImageView
{
	AM_VkImageView()
		: myView(nullptr)
	{
	}

	~AM_VkImageView()
	{
		DestroyView();
	}

	void CreateView(const VkImageViewCreateInfo& aCreateInfo)
	{
		if (vkCreateImageView(AM_VkContext::device, &aCreateInfo, nullptr, &myView) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture image view!");
	}

	void DestroyView()
	{
		vkDestroyImageView(AM_VkContext::device, myView, nullptr);
	}

	VkImageView myView;
};

struct AM_VkRenderPass
{
	AM_VkRenderPass()
		: myPass(nullptr)
	{
	}

	~AM_VkRenderPass()
	{
		vkDestroyRenderPass(AM_VkContext::device, myPass, nullptr);
	}

	void CreateRenderPass(const VkRenderPassCreateInfo& aCreateInfo)
	{
		if (vkCreateRenderPass(AM_VkContext::device, &aCreateInfo, nullptr, &myPass) != VK_SUCCESS)
			throw std::runtime_error("failed to create render pass!");
	}

	VkRenderPass myPass;
};
