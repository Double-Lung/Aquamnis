#include "AM_VkRenderer.h"
#include "vk_mem_alloc.h"
#include <array>

AM_VkRenderer::AM_VkRenderer(AM_VkContext& aVkContext, AM_Window& aWindow, VmaAllocator& aVMA)
	: myVkContext(aVkContext)
	, myWindow(aWindow)
	, myVMA(aVMA)
	, mySwapChain{}
	, myRenderPass{}
	, myColorImageView{}
	, myDepthImageView{}
	, myCurrentFrame(0)
	, myImageIndex(0)
	, myIsFrameStarted(false)
{
	CreateSyncObjects();
	CreateSwapChain();
	CreateRenderPass();
	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();
	CreateReusableCommandBuffers();
}

AM_VkRenderer::~AM_VkRenderer()
{
	FreeCommandBuffers();
	vmaDestroyImage(myVMA, myDepthImage.myImage, myDepthImage.myAllocation);
	vmaDestroyImage(myVMA, myColorImage.myImage, myColorImage.myAllocation);
}

VkCommandBuffer AM_VkRenderer::BeginFrame()
{
	assert(!myIsFrameStarted && "Can't call beginFrame while already in progress");
	vkWaitForFences(myVkContext.device, 1, &myComputeInFlightFences[myCurrentFrame].myFence, VK_TRUE, UINT64_MAX);
	vkWaitForFences(myVkContext.device, 1, &myInFlightFences[myCurrentFrame].myFence, VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(AM_VkContext::device, mySwapChain.GetSwapChain(), UINT64_MAX, myImageAvailableSemaphores[myCurrentFrame].mySemaphore, VK_NULL_HANDLE, &myImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("failed to acquire swap chain image!");

	vkResetFences(AM_VkContext::device, 1, &myComputeInFlightFences[myCurrentFrame].myFence);
	vkResetCommandPool(AM_VkContext::device, myVkContext.myComputeCommandPools[myCurrentFrame].myPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	
	vkResetFences(AM_VkContext::device, 1, &myInFlightFences[myCurrentFrame].myFence);
	vkResetCommandPool(AM_VkContext::device, myVkContext.myCommandPools[myCurrentFrame].myPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	myIsFrameStarted = true;
	VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer!");

	return commandBuffer;
}

void AM_VkRenderer::EndFrame()
{
	assert(myIsFrameStarted && "Can't call endFrame while frame is not in progress");
	VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer!");

	std::array<VkSemaphore, 2> waitSemaphores = { myComputeFinishedSemaphores[myCurrentFrame].mySemaphore, myImageAvailableSemaphores[myCurrentFrame].mySemaphore };
	std::array<VkPipelineStageFlags, 2> waitStages = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &myCommandBuffers[myCurrentFrame];

	std::array<VkSemaphore, 1> signalSemaphores = { myRenderFinishedSemaphores[myCurrentFrame].mySemaphore };
	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	if (vkQueueSubmit(myVkContext.graphicsQueue, 1, &submitInfo, myInFlightFences[myCurrentFrame].myFence) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores.data();

	std::array<VkSwapchainKHR, 1> swapChains = { mySwapChain.GetSwapChain() };
	presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
	presentInfo.pSwapchains = swapChains.data();
	presentInfo.pImageIndices = &myImageIndex;
	presentInfo.pResults = nullptr;
	VkResult result = vkQueuePresentKHR(myVkContext.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || myWindow.WasWindowResized())
	{
		myWindow.ResetResizeFlag();
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swap chain image!");

	myIsFrameStarted = false;
	myCurrentFrame = (myCurrentFrame + 1) % AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT;
}

void AM_VkRenderer::BeginRenderPass(VkCommandBuffer commandBuffer)
{
	assert(myIsFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
	assert(commandBuffer == GetCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = myRenderPass.myPass;
	renderPassInfo.framebuffer = myFramebuffers[myImageIndex].myFramebuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = mySwapChain.GetExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = static_cast<float>(mySwapChain.GetHeight());
	viewport.width = static_cast<float>(mySwapChain.GetWidth());
	viewport.height = -static_cast<float>(mySwapChain.GetHeight());
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{ { 0, 0 }, mySwapChain.GetExtent() };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void AM_VkRenderer::EndRenderPass(VkCommandBuffer commandBuffer)
{
	assert(myIsFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
	assert(commandBuffer == GetCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");
	vkCmdEndRenderPass(commandBuffer);
}

void AM_VkRenderer::SubmitComputeQueue()
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &myComputeCommandBuffers[myCurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &myComputeFinishedSemaphores[myCurrentFrame].mySemaphore;

	if (vkQueueSubmit(myVkContext.computeQueue, 1, &submitInfo, myComputeInFlightFences[myCurrentFrame].myFence) != VK_SUCCESS)
		throw std::runtime_error("failed to submit compute command buffer!");
}

void AM_VkRenderer::CreateReusableCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	myCommandBuffers.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myComputeCommandBuffers.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		allocInfo.commandPool = myVkContext.myCommandPools[i].myPool;
		if (vkAllocateCommandBuffers(AM_VkContext::device, &allocInfo, &myCommandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate command buffer!");

		allocInfo.commandPool = myVkContext.myComputeCommandPools[i].myPool;
		if (vkAllocateCommandBuffers(AM_VkContext::device, &allocInfo, &myComputeCommandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate command buffer!");
	}
}

void AM_VkRenderer::FreeCommandBuffers()
{
	for (int i = 0; i < myVkContext.myCommandPools.size(); ++i)
	{
		vkFreeCommandBuffers(
			myVkContext.device,
			myVkContext.myCommandPools[i].myPool,
			1,
			&myCommandBuffers[i]);
	}
	myCommandBuffers.clear();

	for (int i = 0; i < myVkContext.myComputeCommandPools.size(); ++i)
	{
		vkFreeCommandBuffers(
			myVkContext.device,
			myVkContext.myComputeCommandPools[i].myPool,
			1,
			&myComputeCommandBuffers[i]);
	}
	myComputeCommandBuffers.clear();
}

void AM_VkRenderer::RecreateSwapChain()
{
	int width = 0, height = 0;
	myWindow.WaitForFramebufferSize(width, height);
	vkDeviceWaitIdle(AM_VkContext::device);

	CleanupSwapChain();
	CreateSwapChain();
	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();
}

void AM_VkRenderer::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = myVkContext.surfaceFormat.format;
	colorAttachment.samples = myVkContext.maxMSAASamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = myVkContext.depthFormat;
	depthAttachment.samples = myVkContext.maxMSAASamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = myVkContext.surfaceFormat.format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	myRenderPass.CreateRenderPass(renderPassInfo);
}

void AM_VkRenderer::CreateSwapChain()
{
	int width, height;
	myWindow.GetFramebufferSize(width, height);
	mySwapChain.SetExtent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = myVkContext.surface;
	createInfo.minImageCount = myVkContext.swapChainImageCount;
	createInfo.imageFormat = myVkContext.surfaceFormat.format;
	createInfo.imageColorSpace = myVkContext.surfaceFormat.colorSpace;
	createInfo.imageExtent = mySwapChain.GetExtent();
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	std::vector<uint32_t> queueFamilyIndices = { myVkContext.graphicsFamilyIndex, myVkContext.transferFamilyIndex };
	createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
	createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

	createInfo.preTransform = myVkContext.surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = myVkContext.presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	mySwapChain.Create(createInfo);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = myVkContext.surfaceFormat.format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	mySwapChain.CreateImageViews(viewInfo);
}

void AM_VkRenderer::CleanupSwapChain()
{
	myColorImageView.DestroyView();
	vmaDestroyImage(myVMA, myColorImage.myImage, myColorImage.myAllocation);

	myDepthImageView.DestroyView();
	vmaDestroyImage(myVMA, myDepthImage.myImage, myDepthImage.myAllocation);

	myFramebuffers.clear();
	mySwapChain.Destroy();
}

void AM_VkRenderer::CreateColorResources()
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = mySwapChain.GetExtent().width;
	imageInfo.extent.height = mySwapChain.GetExtent().height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = myVkContext.surfaceFormat.format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageInfo.samples = myVkContext.maxMSAASamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = 0; // Optional

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkResult result = vmaCreateImage(myVMA, &imageInfo, &allocationInfo, &myColorImage.myImage, &myColorImage.myAllocation, nullptr);
	assert(result == VK_SUCCESS && "failed to create image!");

	CreateImageView(myColorImageView, myColorImage.myImage, myVkContext.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void AM_VkRenderer::CreateDepthResources()
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = mySwapChain.GetExtent().width;
	imageInfo.extent.height = mySwapChain.GetExtent().height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = myVkContext.depthFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = myVkContext.maxMSAASamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = 0; // Optional

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkResult result = vmaCreateImage(myVMA, &imageInfo, &allocationInfo, &myDepthImage.myImage, &myDepthImage.myAllocation, nullptr);
	assert(result == VK_SUCCESS && "failed to create image!");

	CreateImageView(myDepthImageView, myDepthImage.myImage, myVkContext.depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	VkCommandBuffer commandBuffer;
	BeginOneTimeCommands(commandBuffer, myVkContext.myCommandPools[myCurrentFrame].myPool);
	TransitionImageLayout(myDepthImage.myImage, myVkContext.depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, commandBuffer);
	EndOneTimeCommands(commandBuffer, myVkContext.graphicsQueue, myVkContext.myCommandPools[myCurrentFrame].myPool);
}

void AM_VkRenderer::CreateFramebuffers()
{
	const auto& swapChainImageViews = mySwapChain.GetImageViews();
	myFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i != swapChainImageViews.size(); ++i)
	{
		std::array<VkImageView, 3> attachments = { myColorImageView.myView, myDepthImageView.myView, swapChainImageViews[i].myView };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = myRenderPass.myPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = mySwapChain.GetWidth();
		framebufferInfo.height = mySwapChain.GetHeight();
		framebufferInfo.layers = 1;

		myFramebuffers[i].CreateFrameBuffer(framebufferInfo);
	}
}

void AM_VkRenderer::CreateSyncObjects()
{
	myInFlightFences.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myImageAvailableSemaphores.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myRenderFinishedSemaphores.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);

	myComputeInFlightFences.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myComputeFinishedSemaphores.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
}

void AM_VkRenderer::CreateImageView(AM_VkImageView& outImageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = aMipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	outImageView.CreateView(viewInfo);
}

void AM_VkRenderer::BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = aCommandPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(AM_VkContext::device, &allocInfo, &aCommandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(aCommandBuffer, &beginInfo);
}

void AM_VkRenderer::EndOneTimeCommands(VkCommandBuffer commandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(aVkQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(aVkQueue);

	vkFreeCommandBuffers(AM_VkContext::device, aCommandPool, 1, &commandBuffer);
}

void AM_VkRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, VkCommandBuffer aCommandBuffer)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = aMipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (HasStencilComponent(format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		throw std::invalid_argument("unsupported layout transition!");

	vkCmdPipelineBarrier(
		aCommandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

bool AM_VkRenderer::HasStencilComponent(VkFormat format)
{
	return !((format ^ VK_FORMAT_D32_SFLOAT_S8_UINT) && (format ^ VK_FORMAT_D24_UNORM_S8_UINT));
}
