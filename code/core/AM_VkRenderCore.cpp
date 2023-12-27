#define STB_IMAGE_IMPLEMENTATION
#include "AM_VkRenderCore.h"
#include "AM_VkRenderer.h"
#include "AM_SimpleRenderSystem.h"
#include "AM_PointLightRenderSystem.h"
#include "AM_Camera.h"
#include "AM_SimpleTimer.h"
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <unordered_map>

void AM_VkRenderCore::Engage()
{
	InitVulkan();
	MainLoop();
}

AM_VkRenderCore::AM_VkRenderCore() 
	: myVkContext{}
	, myDescriptorPool{myVkContext}
	, myMipLevels(0)
{
}

AM_VkRenderCore::~AM_VkRenderCore()
{
	delete myPointLightRenderSystem;
	delete myRenderSystem;
	delete myRenderer;
}

bool AM_VkRenderCore::CheckExtensionSupport()
{
#ifdef _DEBUG
	std::unordered_set<std::string> requiredExtensionSet(myVkContext.requiredInstanceExtensions.cbegin(), myVkContext.requiredInstanceExtensions.cend());
	std::cout << "Extension Status:\n";
	for (const auto& extension : myVkContext.availableInstanceExtensions)
	{
		std::cout << '\t' << extension.extensionName;
		if (requiredExtensionSet.find(extension.extensionName) != requiredExtensionSet.cend())
			std::cout << ": Yes" << '\n';
		else
			std::cout << ": No" << '\n';
	}
#endif

	std::unordered_set<std::string> availableExtensionSet;
	for (const auto& extension : myVkContext.availableInstanceExtensions)
		availableExtensionSet.insert(extension.extensionName);

	for (const char* extensionName : myVkContext.requiredInstanceExtensions) 
		if (availableExtensionSet.find(extensionName) == availableExtensionSet.cend())
			return false;

	return true;
}

bool AM_VkRenderCore::CheckInstanceLayerSupport()
{
	std::unordered_set<std::string> availableLayerSet;
	for (const auto& layerProperties : myVkContext.availableInstanceLayers)
		availableLayerSet.insert(layerProperties.layerName);

	for (const char* layerName : myVkContext.enabledInstanceLayers) 
		if (availableLayerSet.find(layerName) == availableLayerSet.cend())
			return false;
	return true;
}

void AM_VkRenderCore::CreateInstance()
{
	if (!CheckExtensionSupport())
		throw std::runtime_error("extensions requested by GLFW, but not available!");

	if (!CheckInstanceLayerSupport())
		throw std::runtime_error("layers requested by application, but not available!");

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = ApplicationConstants::WINDOWNAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = ApplicationConstants::WINDOWNAME;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(myVkContext.requiredInstanceExtensions.size());
	createInfo.ppEnabledExtensionNames = myVkContext.requiredInstanceExtensions.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(myVkContext.enabledInstanceLayers.size());
	createInfo.ppEnabledLayerNames = myVkContext.enabledInstanceLayers.data();

#ifdef _DEBUG
	std::vector<VkValidationFeatureEnableEXT> enables =
	{ VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };

	VkValidationFeaturesEXT features = {};
	features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	features.enabledValidationFeatureCount = static_cast<uint32_t>(enables.size());
	features.pEnabledValidationFeatures = enables.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	myVkContext.PopulateDebugMessengerCreateInfo(debugCreateInfo);
	debugCreateInfo.pNext = &features;
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &AM_VkContext::instance) != VK_SUCCESS)
		throw std::runtime_error("failed to create Vulkan instance!");

	if (glfwCreateWindowSurface(AM_VkContext::instance, myWindowInstance.GetWindow(), nullptr, &AM_VkContext::surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");
}

void AM_VkRenderCore::CreateImageView(AM_VkImageView& outImageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels)
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

void AM_VkRenderCore::CreateDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.resize(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);

	myDescriptorPool.CreatePool(static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT), 0, poolSizes);
}

void AM_VkRenderCore::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT, myRenderSystem->GetDescriptorSetLayout());
	myDescriptorSets.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myDescriptorPool.AllocateDescriptorSets(layouts, myDescriptorSets);
	
	for (size_t i = 0; i < AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = myVirtualUniformBuffer->myBuffer;
		bufferInfo.offset = i * 0x100;
		bufferInfo.range = sizeof(UniformBufferObject); // or VK_WHOLE_SIZE
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = myTextureImageView.myView;
		imageInfo.sampler = myTextureSampler.mySampler;

		AM_VkDescriptorSetWriter writter{ myRenderSystem->GetDescriptorSetLayoutWrapper(), myDescriptorPool };
		writter.WriteBuffer(0, &bufferInfo);
		writter.WriteImage(1, &imageInfo);
		writter.Update(myDescriptorSets[i]);
	}
}

void AM_VkRenderCore::CreateTextureImageView()
{
	CreateImageView(myTextureImageView, myTextureImage->GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, myMipLevels);
}

AM_Image* AM_VkRenderCore::CreateImage(const VkExtent2D& anExtent, uint32_t aMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = anExtent.width;
	imageInfo.extent.height = anExtent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = aMipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = 0; // Optional

	return myMemoryAllocator.AllocateImage(imageInfo, properties);
}

void AM_VkRenderCore::CreateTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(AM_VkRenderCoreConstants::TEXTURE_PATH, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	myMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	VkDeviceSize imageSize = (uint64_t)texWidth * (uint64_t)texHeight * 4;

	AM_Buffer* stagingBuffer = myMemoryAllocator.AllocateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	myMemoryAllocator.CopyToMappedMemory(*stagingBuffer, (void*)pixels, static_cast<size_t>(imageSize));
	stbi_image_free(pixels);

	myTextureImage = CreateImage({ (uint32_t)texWidth, (uint32_t)texHeight }, myMipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkCommandBuffer commandBuffer;
	BeginOneTimeCommands(commandBuffer, myVkContext.myTransferCommandPool.myPool);
	TransitionImageLayout(myTextureImage->GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, myMipLevels, commandBuffer);
	CopyBufferToImage(*stagingBuffer, myTextureImage->GetImage(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), commandBuffer);

	VkImageMemoryBarrier postCopyTransferMemoryBarrier{};
	postCopyTransferMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	postCopyTransferMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	postCopyTransferMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	postCopyTransferMemoryBarrier.srcQueueFamilyIndex = myVkContext.transferFamilyIndex;
	postCopyTransferMemoryBarrier.dstQueueFamilyIndex = myVkContext.graphicsFamilyIndex;
	postCopyTransferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	postCopyTransferMemoryBarrier.dstAccessMask = 0;
	postCopyTransferMemoryBarrier.image = myTextureImage->GetImage();
	postCopyTransferMemoryBarrier.subresourceRange.baseMipLevel = 0;
	postCopyTransferMemoryBarrier.subresourceRange.levelCount = myMipLevels;
	postCopyTransferMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	postCopyTransferMemoryBarrier.subresourceRange.layerCount = 1;
	postCopyTransferMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &postCopyTransferMemoryBarrier
	);

	BeginOwnershipTransfer(commandBuffer, myVkContext.transferQueue, myTransferSemaphores[0].mySemaphore);

	VkCommandBuffer graphicsCommandBuffer;
	BeginOneTimeCommands(graphicsCommandBuffer, myVkContext.myCommandPools[0].myPool);

	VkImageMemoryBarrier postCopyGraphicsMemoryBarrier{};
	postCopyGraphicsMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	postCopyGraphicsMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	postCopyGraphicsMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	postCopyGraphicsMemoryBarrier.srcQueueFamilyIndex = myVkContext.transferFamilyIndex;
	postCopyGraphicsMemoryBarrier.dstQueueFamilyIndex = myVkContext.graphicsFamilyIndex;
	postCopyGraphicsMemoryBarrier.srcAccessMask = 0;
	postCopyGraphicsMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	postCopyGraphicsMemoryBarrier.image = myTextureImage->GetImage();
	postCopyGraphicsMemoryBarrier.subresourceRange.baseMipLevel = 0;
	postCopyGraphicsMemoryBarrier.subresourceRange.levelCount = myMipLevels;
	postCopyGraphicsMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	postCopyGraphicsMemoryBarrier.subresourceRange.layerCount = 1;
	postCopyGraphicsMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	vkCmdPipelineBarrier(
		graphicsCommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &postCopyGraphicsMemoryBarrier
	);

	EndOwnershipTransfer(graphicsCommandBuffer, myVkContext.graphicsQueue, myTransferSemaphores[0].mySemaphore);

	// this is good for now
	// use fence for async submission
	vkQueueWaitIdle(myVkContext.transferQueue);
	vkFreeCommandBuffers(AM_VkContext::device, myVkContext.myTransferCommandPool.myPool, 1, &commandBuffer);

	vkQueueWaitIdle(myVkContext.graphicsQueue);
	vkFreeCommandBuffers(AM_VkContext::device, myVkContext.myCommandPools[0].myPool, 1, &graphicsCommandBuffer);

	GenerateMipmaps(myTextureImage->GetImage(), VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, myMipLevels);
	stagingBuffer->SetIsEmpty(true);
}

void AM_VkRenderCore::CopyBufferToImage(AM_Buffer& aBuffer, VkImage anImage, const uint32_t aWidth, const uint32_t aHeight, VkCommandBuffer aCommandBuffer)
{
	VkBufferImageCopy region{};
	region.bufferOffset = aBuffer.GetOffset();
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { aWidth, aHeight, 1 };

	vkCmdCopyBufferToImage(
		aCommandBuffer,
		aBuffer.myBuffer,
		anImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
}

void AM_VkRenderCore::CopyBuffer(AM_Buffer& aSourceBuffer, AM_Buffer& aDestinationBuffer, const VkDeviceSize aSize)
{
	VkCommandBuffer commandBuffer;
	BeginOneTimeCommands(commandBuffer, myVkContext.myTransferCommandPool.myPool);

	VkBufferCopy copyRegion{ aSourceBuffer.GetOffset(), aDestinationBuffer.GetOffset(), aSize };
	vkCmdCopyBuffer(commandBuffer, aSourceBuffer.myBuffer, aDestinationBuffer.myBuffer, 1, &copyRegion);

	VkBufferMemoryBarrier bufferMemoryBarrier{};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	bufferMemoryBarrier.dstAccessMask = 0;
	bufferMemoryBarrier.srcQueueFamilyIndex = myVkContext.transferFamilyIndex;
	bufferMemoryBarrier.dstQueueFamilyIndex = myVkContext.graphicsFamilyIndex;
	bufferMemoryBarrier.buffer = aDestinationBuffer.myBuffer;
	bufferMemoryBarrier.offset = aDestinationBuffer.GetOffset();
	bufferMemoryBarrier.size = aSize;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,      // srcStageMask
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
		0,
		0, nullptr,  
		1, &bufferMemoryBarrier,
		0, nullptr
	);

	BeginOwnershipTransfer(commandBuffer, myVkContext.transferQueue, myTransferSemaphores[0].mySemaphore);

	VkCommandBuffer graphicsCommandBuffer;
	BeginOneTimeCommands(graphicsCommandBuffer, myVkContext.myCommandPools[0].myPool);

	bufferMemoryBarrier.srcAccessMask = 0;
	bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

	vkCmdPipelineBarrier(
		graphicsCommandBuffer, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		0,
		0, nullptr,
		1, &bufferMemoryBarrier,               
		0, nullptr
	);

	EndOwnershipTransfer(graphicsCommandBuffer, myVkContext.graphicsQueue, myTransferSemaphores[0].mySemaphore);

	vkQueueWaitIdle(myVkContext.transferQueue);
	vkFreeCommandBuffers(AM_VkContext::device, myVkContext.myTransferCommandPool.myPool, 1, &commandBuffer);

	vkQueueWaitIdle(myVkContext.graphicsQueue);
	vkFreeCommandBuffers(AM_VkContext::device, myVkContext.myCommandPools[0].myPool, 1, &graphicsCommandBuffer);
}

void AM_VkRenderCore::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, VkCommandBuffer aCommandBuffer)
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


void AM_VkRenderCore::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t aMipLevels)
{
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(AM_VkContext::physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("texture image format does not support linear blitting!");

	// It should be noted that it is uncommon in practice to generate the mipmap levels at runtime anyway. 
	// Usually they are pre-generated and stored in the texture file alongside the base level to improve loading speed. 
	// Implementing resizing in software and loading multiple levels from a file is left as an exercise to the reader.

	VkCommandBuffer commandBuffer;
	BeginOneTimeCommands(commandBuffer, myVkContext.myCommandPools[0].myPool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;

	barrier.subresourceRange.levelCount = aMipLevels;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	// transfer image back to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to avoid messy code
	// since we are not going to generate mipmap at runtime in the future
	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;
	for (uint32_t i = 1; i < aMipLevels; i++) 
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = myMipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	EndOneTimeCommands(commandBuffer, myVkContext.graphicsQueue, myVkContext.myCommandPools[0].myPool);
}

void AM_VkRenderCore::CreateVertexBuffer(AM_Entity& anEntity)
{
	VkDeviceSize bufferSize = sizeof(anEntity.GetVertices()[0]) * anEntity.GetVertices().size();
	AM_Buffer* stagingBuffer = myMemoryAllocator.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	myMemoryAllocator.CopyToMappedMemory(*stagingBuffer, (void*)anEntity.GetVertices().data(), static_cast<size_t>(bufferSize));
	anEntity.SetVertexBuffer(myMemoryAllocator.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	CopyBuffer(*stagingBuffer, *anEntity.GetVertexBuffer(), bufferSize);
	stagingBuffer->SetIsEmpty(true);
}

void AM_VkRenderCore::CreateIndexBuffer(AM_Entity& anEntity)
{
	VkDeviceSize bufferSize = sizeof(anEntity.GetIndices()[0]) * anEntity.GetIndices().size();
	AM_Buffer* stagingBuffer = myMemoryAllocator.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	myMemoryAllocator.CopyToMappedMemory(*stagingBuffer, (void*)anEntity.GetIndices().data(), static_cast<size_t>(bufferSize));
	anEntity.SetIndexBuffer(myMemoryAllocator.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	CopyBuffer(*stagingBuffer, *anEntity.GetIndexBuffer(), bufferSize);
	stagingBuffer->SetIsEmpty(true);
}

void AM_VkRenderCore::CreateUniformBuffers()
{
	static constexpr uint64_t bufferSize = 0x100 * AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT;
	myVirtualUniformBuffer = myMemoryAllocator.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void AM_VkRenderCore::UpdateUniformBuffer(uint32_t currentImage, const AM_Camera& aCamera)
{
	UniformBufferObject ubo{};
	ubo.view = aCamera.GetViewMatrix();
	ubo.projection = aCamera.GetProjectionMatrix();
	ubo.ambientColor = { 1.f, 1.f, 1.f, 0.03f };
	ubo.lightColor = { 1.f, 0.2f, 0.f, 1.f };
	ubo.lightPosition = { -8.f, 10.f, 0.f };

	char* mappedUniformBuffers = (char*) myVirtualUniformBuffer->GetMappedMemory();
	assert(mappedUniformBuffers != nullptr&& "Uniform buffer is not mapped!");

	char* destination = mappedUniformBuffers + currentImage * 0x100;
	memcpy((void*)destination, &ubo, sizeof(ubo));
}

void AM_VkRenderCore::BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool)
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

void AM_VkRenderCore::BeginOwnershipTransfer(VkCommandBuffer& aSrcCommandBuffer, VkQueue& aSrcQueue, VkSemaphore& aSignalSemaphore)
{
	vkEndCommandBuffer(aSrcCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &aSrcCommandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &aSignalSemaphore;

	vkQueueSubmit(aSrcQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

void AM_VkRenderCore::EndOwnershipTransfer(VkCommandBuffer& aDstCommandBuffer, VkQueue& aDstQueue, VkSemaphore& aWaitSemaphore)
{
	vkEndCommandBuffer(aDstCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &aDstCommandBuffer;
	
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &aWaitSemaphore;
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	submitInfo.pWaitDstStageMask = waitStages;

	vkQueueSubmit(aDstQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

void AM_VkRenderCore::EndOneTimeCommands(VkCommandBuffer commandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool)
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

void AM_VkRenderCore::CreateSyncObjects()
{
	mySyncFences.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myTransferSemaphores.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
}

void AM_VkRenderCore::LoadEntities()
{
 	AM_Entity vikingRoomEntity = AM_Entity::CreateEntity();
 	auto& transform1 = vikingRoomEntity.GetTransformComponent();
 	transform1.myTranslation = { 12.f, 0.f, 0.f };
 	vikingRoomEntity.LoadModel(AM_VkRenderCoreConstants::MODEL_PATH);
 	CreateVertexBuffer(vikingRoomEntity);
 	CreateIndexBuffer(vikingRoomEntity);
	myEntities.emplace(vikingRoomEntity.GetId(), std::move(vikingRoomEntity));

	AM_Entity vaseEntity = AM_Entity::CreateEntity();
	auto& transform2 = vaseEntity.GetTransformComponent();
	transform2.myTranslation = { -8.f, 0.f, 0.f };
	transform2.myRotation = { 3.14159f, 0.f, 0.f};
	transform2.myScale = { 20.f, 20.f, 20.f };
	vaseEntity.LoadModel("../data/models/smooth_vase.obj");
	CreateVertexBuffer(vaseEntity);
	CreateIndexBuffer(vaseEntity);
	myEntities.emplace(vaseEntity.GetId(), std::move(vaseEntity));

	AM_Entity quadEntity = AM_Entity::CreateEntity();
	auto& transform3 = quadEntity.GetTransformComponent();
	transform3.myTranslation = { 0.f, -1.f, 0.f };
	transform3.myScale = { 42.f, 1.f, 42.f };
	quadEntity.LoadModel("../data/models/quad.obj");
	CreateVertexBuffer(quadEntity);
	CreateIndexBuffer(quadEntity);
	myEntities.emplace(quadEntity.GetId(), std::move(quadEntity));
}

void AM_VkRenderCore::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = myVkContext.deviceProperties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = static_cast<float>(myMipLevels); // or VK_LOD_CLAMP_NONE

	myTextureSampler.CreateSampler(samplerInfo);
}

bool AM_VkRenderCore::HasStencilComponent(VkFormat format)
{
	return !((format ^ VK_FORMAT_D32_SFLOAT_S8_UINT) && (format ^ VK_FORMAT_D24_UNORM_S8_UINT));
}

void AM_VkRenderCore::InitVulkan()
{
	myWindowInstance.Init();
	CreateInstance();
	myVkContext.Init();
	myMemoryAllocator.Init(myVkContext.memoryProperties);
	CreateSyncObjects();

	myRenderer = new AM_VkRenderer(myVkContext, myWindowInstance, myMemoryAllocator);

	// load textures
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();

	// load 3d models
	LoadEntities();
	CreateUniformBuffers();

	myRenderSystem = new AM_SimpleRenderSystem(myVkContext, myRenderer->GetRenderPass());
	myPointLightRenderSystem = new AM_PointLightRenderSystem(myVkContext, myRenderer->GetRenderPass());
	
	CreateDescriptorPool();
	CreateDescriptorSets();
}

void AM_VkRenderCore::MainLoop()
{
	AM_Camera camera;
	camera.myTransformComp.myTranslation = { 0.f, 15.f, 35.f };
	camera.myTransformComp.myRotation = { 0.f, 0.f, 0.f };
	camera.SetPerspectiveProjection(0.7854f, myRenderer->GetAspectRatio(), 0.1f, 100.f);
	camera.SetRotation(camera.myTransformComp.myTranslation, camera.myTransformComp.myRotation);

	while (!myWindowInstance.ShouldCloseWindow())
	{
		glfwPollEvents();
		float deltaTime = AM_SimpleTimer::GetInstance().GetDeltaTime();
		if (auto commandBufer = myRenderer->BeginFrame())
		{
			myRenderer->BeginRenderPass(commandBufer);
			UpdateCameraTransform(deltaTime, camera);
			UpdateUniformBuffer(myRenderer->GetFrameIndex(), camera);

			myRenderSystem->RenderEntities(commandBufer, myDescriptorSets[myRenderer->GetFrameIndex()], myEntities, camera);
			myPointLightRenderSystem->Render(commandBufer, myDescriptorSets[myRenderer->GetFrameIndex()], myEntities, camera);

			myRenderer->EndRenderPass(commandBufer);
			myRenderer->EndFrame();

			if (myWindowInstance.ShouldUpdateCamera())
			{
				camera.SetPerspectiveProjection(0.7854f, myRenderer->GetAspectRatio(), 0.1f, 100.f);
				myWindowInstance.ResetCameraUpdateFlag();
			}
		}
	}

	vkDeviceWaitIdle(AM_VkContext::device);
}

void AM_VkRenderCore::UpdateCameraTransform(float aDeltaTime, AM_Camera& aCamera)
{
	bool rotationChanged = false;
	glm::vec3 rotation{ 0.f };
	if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		rotationChanged = true;
		rotation.y += 1.f;
	}
	else if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		rotationChanged = true;
		rotation.y -= 1.f;
	}
	if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_UP) == GLFW_PRESS)
	{
		rotationChanged = true;
		rotation.x += 1.f;
	}
	else if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		rotationChanged = true;
		rotation.x -= 1.f;
	}
	if (rotationChanged)
		aCamera.myTransformComp.myRotation += 1.5f * aDeltaTime * glm::normalize(rotation);

	aCamera.myTransformComp.myRotation.x = glm::clamp(aCamera.myTransformComp.myRotation.x, -1.5f, 1.5f);
	aCamera.myTransformComp.myRotation.y = glm::mod(aCamera.myTransformComp.myRotation.y, glm::two_pi<float>());

	const float yaw = aCamera.myTransformComp.myRotation.y;
	glm::vec3 forwardDir{ -sin(yaw), 0.f, -cos(yaw) }; // camera is facing -z axis by default
	glm::vec3 rightDir{ -forwardDir.z, 0.f, forwardDir.x };
	glm::vec3 upDir{ 0.f, 1.f, 0.f };

 	bool translateChanged = false;
	glm::vec3 translate{ 0.f };
	if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_A) == GLFW_PRESS)
	{
		translateChanged = true;
		translate -= rightDir;
	} 
	else if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_D) == GLFW_PRESS)
	{
		translateChanged = true;
		translate += rightDir;
	}
	if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
	{
		translateChanged = true;
		translate += forwardDir;
	}
	else if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_S) == GLFW_PRESS)
	{
		translateChanged = true;
		translate -= forwardDir;
	}
	if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_Q) == GLFW_PRESS)
	{
		translateChanged = true;
		translate += upDir;
	}
	else if (glfwGetKey(myWindowInstance.GetWindow(), GLFW_KEY_E) == GLFW_PRESS)
	{
		translateChanged = true;
		translate -= upDir;
	}

	if (translateChanged)
		aCamera.myTransformComp.myTranslation += 5.f * aDeltaTime * glm::normalize(translate);

	if (translateChanged || rotationChanged)
		aCamera.SetRotation(aCamera.myTransformComp.myTranslation, aCamera.myTransformComp.myRotation);
}

