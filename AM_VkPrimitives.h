#pragma once
#include "VkDrawContext.h"

struct AM_VkSemaphore
{
	AM_VkSemaphore()
		: mySemaphore(nullptr)
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(VkDrawContext::device, &semaphoreInfo, nullptr, &mySemaphore) != VK_SUCCESS)
			throw std::runtime_error("failed to create semaphores!");
	}
	~AM_VkSemaphore()
	{
		vkDestroySemaphore(VkDrawContext::device, mySemaphore, nullptr);
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

		if (vkCreateFence(VkDrawContext::device, &fenceInfo, nullptr, &myFence) != VK_SUCCESS)
			throw std::runtime_error("failed to create semaphores!");
	}

	~AM_VkFence()
	{
		vkDestroyFence(VkDrawContext::device, myFence, nullptr);
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
		vkDestroyPipelineLayout(VkDrawContext::device, myLayout, nullptr);
	}

	void CreateLayout(const VkPipelineLayoutCreateInfo& aCreateInfo)
	{
		if (vkCreatePipelineLayout(VkDrawContext::device, &aCreateInfo, nullptr, &myLayout) != VK_SUCCESS)
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
		vkDestroyPipeline(VkDrawContext::device, myPipeline, nullptr);
	}

	void CreatePipeline(const VkGraphicsPipelineCreateInfo& aCreateInfo)
	{
		if (vkCreateGraphicsPipelines(VkDrawContext::device, VK_NULL_HANDLE, 1, &aCreateInfo, nullptr, &myPipeline) != VK_SUCCESS)
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
		if (vkCreateFramebuffer(VkDrawContext::device, &aCreateInfo, nullptr, &myFramebuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}

	void DestroyFrameBuffer()
	{
		vkDestroyFramebuffer(VkDrawContext::device, myFramebuffer, nullptr);
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
		if (vkAllocateCommandBuffers(VkDrawContext::device, &aCreateInfo, &myCommandBuffer) != VK_SUCCESS)
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
		vkDestroySampler(VkDrawContext::device, mySampler, nullptr);
	}

	void CreateSampler(const VkSamplerCreateInfo& aCreateInfo)
	{
		if (vkCreateSampler(VkDrawContext::device, &aCreateInfo, nullptr, &mySampler) != VK_SUCCESS)
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
		if (vkCreateImageView(VkDrawContext::device, &aCreateInfo, nullptr, &myView) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture image view!");
	}

	void DestroyView()
	{
		vkDestroyImageView(VkDrawContext::device, myView, nullptr);
	}

	VkImageView myView;
};

struct AM_VkDescriptorSetLayout
{
	AM_VkDescriptorSetLayout()
		: myLayout(nullptr)
	{
	}

	~AM_VkDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(VkDrawContext::device, myLayout, nullptr);
	}

	void CreateLayout(const VkDescriptorSetLayoutCreateInfo& aCreateInfo)
	{
		if (vkCreateDescriptorSetLayout(VkDrawContext::device, &aCreateInfo, nullptr, &myLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkDescriptorSetLayout myLayout;
};


struct AM_VkRenderPass
{
	AM_VkRenderPass()
		: myPass(nullptr)
	{
	}

	~AM_VkRenderPass()
	{
		vkDestroyRenderPass(VkDrawContext::device, myPass, nullptr);
	}

	void CreateRenderPass(const VkRenderPassCreateInfo& aCreateInfo)
	{
		if (vkCreateRenderPass(VkDrawContext::device, &aCreateInfo, nullptr, &myPass) != VK_SUCCESS)
			throw std::runtime_error("failed to create render pass!");
	}

	VkRenderPass myPass;
};

struct AM_VkDescriptorPool
{
	AM_VkDescriptorPool()
		: myPool(nullptr)
	{
	}

	~AM_VkDescriptorPool()
	{
		vkDestroyDescriptorPool(VkDrawContext::device, myPool, nullptr);
	}

	void CreateDescriptorPool(const VkDescriptorPoolCreateInfo& aCreateInfo)
	{
		if (vkCreateDescriptorPool(VkDrawContext::device, &aCreateInfo, nullptr, &myPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create descriptor pool!");
	}

	VkDescriptorPool myPool;
};

struct AM_VkCommandPool
{
	AM_VkCommandPool()
		: myPool(nullptr)
	{
	}

	~AM_VkCommandPool()
	{
		vkDestroyCommandPool(VkDrawContext::device, myPool, nullptr);
	}
	
	void CreatePool(const VkCommandPoolCreateInfo& aCreateInfo)
	{
		if (vkCreateCommandPool(VkDrawContext::device, &aCreateInfo, nullptr, &myPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create command pool!");
	}

	VkCommandPool myPool;
};
