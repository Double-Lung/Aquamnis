#include "AM_VkRenderCore.h"
#include "AM_VkRenderContext.h"
#include "AM_VkRenderMethodBillboard.h"
#include "AM_VkRenderMethodCubeMap.h"
#include "AM_VkRenderMethodMesh.h"
#include "AM_VkRenderMethodPoint.h"
#include "AM_Camera.h"
#include "AM_SimpleTimer.h"
#include "AM_Particle.h"
#include "AM_VertexInfo.h"
#include "AM_Texture.h"
#include "AM_VkDescriptorSetWritesBuilder.h"
#include "AM_VmaUsage.h"
#include "AM_Entity.h"
#include "AM_EntityStorage.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <tiny_obj_loader.h>
#include <stb_image.h>
#include <random>
#include <unordered_map>

AM_VkRenderCore::AM_VkRenderCore()
	: myVkContext{}
	, myMipLevels(0)
	, myCubeMapMipLevels(0)
{
}

AM_VkRenderCore::~AM_VkRenderCore()
{
	delete myCubeMapRenderMethod;
	delete myBillboardRenderMethod;
	delete myMeshRenderMethod;
	delete myRenderContext;

	for (auto& entity : myEntities)
	{
		if (auto* indexBuffer = entity.second.GetTempIndexBuffer())
			vmaDestroyBuffer(myVMA, indexBuffer->myBuffer, indexBuffer->myAllocation);
		if (auto* vertexBuffer = entity.second.GetTempVertexBuffer())
			vmaDestroyBuffer(myVMA, vertexBuffer->myBuffer, vertexBuffer->myAllocation);
	}

	vmaDestroyBuffer(myVMA, myUniformBuffer.myBuffer, myUniformBuffer.myAllocation);
	vmaDestroyImage(myVMA, myTextureImage.myImage, myTextureImage.myAllocation);
	vmaDestroyImage(myVMA, myCubeMapImage.myImage, myCubeMapImage.myAllocation);
	vmaDestroyAllocator(myVMA);

	myVkContext.DestroyImageView(myCubeMapImageView);
	myVkContext.DestroyImageView(myTextureImageView);

	myVkContext.DestroySampler(myTextureSampler);
	myVkContext.DestroySampler(myCubeMapSampler);

	myVkContext.DestroyDescriptorPool(myGlobalDescriptorPool);

	for (VkSemaphore semaphore : myTransferSemaphores)
		myVkContext.DestroySemaphore(semaphore);
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

void AM_VkRenderCore::CreateImageView(VkImageView& outImageView, VkImage image, VkFormat format, VkImageViewType aViewType, VkImageAspectFlags aspectFlags, uint32_t aMipLevels, uint32_t aLayerCount)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = aViewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = aMipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = aLayerCount;

	outImageView = myVkContext.CreateImageView(viewInfo);
}

void AM_VkRenderCore::AllocatePerEntityDescriptorSets(AM_Entity& outEntity)
{
	std::vector<VkDescriptorSetLayout> layouts(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT, myMeshRenderMethod->GetDescriptorSetLayout());
	std::vector<VkDescriptorSet>& descriptorSets = outEntity.GetDescriptorSets();
	myVkContext.AllocateDescriptorSets(myGlobalDescriptorPool, layouts, descriptorSets);

	for (size_t i = 0; i < AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = outEntity.GetUniformBuffer()->myBuffer;
		bufferInfo.offset = i * AM_VkRenderCoreConstants::UBO_ALIGNMENT;
		bufferInfo.range = sizeof(AM_Entity::EntityUBO); // or VK_WHOLE_SIZE

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = outEntity.GetTexture().myImageView;
		imageInfo.sampler = outEntity.GetTexture().mySampler;

		AM_VkDescriptorSetWritesBuilder writter{ myGlobalDescriptorPool };
		writter.WriteBuffer(0, &bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writter.WriteImage(1, &imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		myVkContext.WriteToDescriptorSet(descriptorSets[i], writter.GetWrites());
	}
}

void AM_VkRenderCore::CreateTextureImage(TempImage& outImage, const char** somePaths, uint32_t aLayerCount = 1) // #FIX_ME: move to a common util class
{
	int texWidth = 0, texHeight = 0, texChannels = 0;
	std::vector<void*> imageLayers(aLayerCount, nullptr);
	for (uint32_t i = 0; i < aLayerCount; ++i)
	{
		imageLayers[i] = reinterpret_cast<void*>(stbi_load(somePaths[i], &texWidth, &texHeight, &texChannels, STBI_rgb_alpha));
		if (!imageLayers[i])
			throw std::runtime_error("failed to load texture image!");
		assert(texWidth && texHeight && texChannels);
	}

	//myMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;  // #FIX_ME: add back mipmap
	uint64_t stride = static_cast<uint64_t>(texWidth) * static_cast<uint64_t>(texHeight) * static_cast<uint64_t>(texChannels);
	uint64_t bufferSize = stride * static_cast<uint64_t>(aLayerCount);

	TempBuffer stagingBuffer;
	CreateFilledStagingBuffer(stagingBuffer, bufferSize, stride, imageLayers);
	for (void* dataPtr : imageLayers)
		stbi_image_free(reinterpret_cast<stbi_uc*>(dataPtr));

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = (uint32_t)texWidth;
	imageInfo.extent.height = (uint32_t)texHeight;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = myMipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = 0; // Optional

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkResult result = vmaCreateImage(myVMA, &imageInfo, &allocationInfo, &outImage.myImage, &outImage.myAllocation, nullptr);
	assert(result == VK_SUCCESS && "failed to create image!");

	VkCommandBuffer commandBuffer;
	BeginOneTimeCommands(commandBuffer, myVkContext.myTransferCommandPool);
	TransitionImageLayout(outImage.myImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, myMipLevels, aLayerCount, commandBuffer);
	CopyBufferToImage(stagingBuffer.myBuffer, outImage.myImage, texWidth, texHeight, aLayerCount, commandBuffer);

	VkImageMemoryBarrier postCopyTransferMemoryBarrier{};
	postCopyTransferMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	postCopyTransferMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	postCopyTransferMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	postCopyTransferMemoryBarrier.srcQueueFamilyIndex = myVkContext.transferFamilyIndex;
	postCopyTransferMemoryBarrier.dstQueueFamilyIndex = myVkContext.graphicsFamilyIndex;
	postCopyTransferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	postCopyTransferMemoryBarrier.dstAccessMask = 0;
	postCopyTransferMemoryBarrier.image = outImage.myImage;
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

	BeginOwnershipTransfer(commandBuffer, myVkContext.transferQueue, myTransferSemaphores[0]);

	VkCommandBuffer graphicsCommandBuffer;
	BeginOneTimeCommands(graphicsCommandBuffer, myVkContext.myCommandPools[0]);

	VkImageMemoryBarrier postCopyGraphicsMemoryBarrier{};
	postCopyGraphicsMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	postCopyGraphicsMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	postCopyGraphicsMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	postCopyGraphicsMemoryBarrier.srcQueueFamilyIndex = myVkContext.transferFamilyIndex;
	postCopyGraphicsMemoryBarrier.dstQueueFamilyIndex = myVkContext.graphicsFamilyIndex;
	postCopyGraphicsMemoryBarrier.srcAccessMask = 0;
	postCopyGraphicsMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	postCopyGraphicsMemoryBarrier.image = outImage.myImage;
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

	EndOwnershipTransfer(graphicsCommandBuffer, myVkContext.graphicsQueue, myTransferSemaphores[0]);

	// this is good for now
	// use fence for async submission
	vkQueueWaitIdle(myVkContext.transferQueue);
	vkFreeCommandBuffers(myVkContext.device, myVkContext.myTransferCommandPool, 1, &commandBuffer);

	vkQueueWaitIdle(myVkContext.graphicsQueue);
	vkFreeCommandBuffers(myVkContext.device, myVkContext.myCommandPools[0], 1, &graphicsCommandBuffer);

	// GenerateMipmaps(myTextureImage.myImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, myMipLevels);
	vmaDestroyBuffer(myVMA, stagingBuffer.myBuffer, stagingBuffer.myAllocation);
}

void AM_VkRenderCore::CopyBufferToImage(VkBuffer aSourceBuffer, VkImage anImage, uint32_t aWidth, uint32_t aHeight, uint32_t aLayerCount, VkCommandBuffer aCommandBuffer)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = aLayerCount;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { aWidth, aHeight, 1 };

	vkCmdCopyBufferToImage(
		aCommandBuffer,
		aSourceBuffer,
		anImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
}

void AM_VkRenderCore::CopyBuffer(VkBuffer aSourceBuffer, VmaAllocationInfo* anAllocationInfo, const TempBuffer* aDestinationBuffer)
{
	VkCommandBuffer commandBuffer;
	BeginOneTimeCommands(commandBuffer, myVkContext.myTransferCommandPool);

	VkBufferCopy copyRegion{ 0, 0, anAllocationInfo->size };
	vkCmdCopyBuffer(commandBuffer, aSourceBuffer, aDestinationBuffer->myBuffer, 1, &copyRegion);

	VkBufferMemoryBarrier bufferMemoryBarrier{};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	bufferMemoryBarrier.dstAccessMask = 0;
	bufferMemoryBarrier.srcQueueFamilyIndex = myVkContext.transferFamilyIndex;
	bufferMemoryBarrier.dstQueueFamilyIndex = myVkContext.graphicsFamilyIndex;
	bufferMemoryBarrier.buffer = aDestinationBuffer->myBuffer;
	bufferMemoryBarrier.offset = 0;
	bufferMemoryBarrier.size = anAllocationInfo->size;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,      // srcStageMask
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
		0,
		0, nullptr,
		1, &bufferMemoryBarrier,
		0, nullptr
	);

	BeginOwnershipTransfer(commandBuffer, myVkContext.transferQueue, myTransferSemaphores[0]);

	VkCommandBuffer graphicsCommandBuffer;
	BeginOneTimeCommands(graphicsCommandBuffer, myVkContext.myCommandPools[0]);

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

	EndOwnershipTransfer(graphicsCommandBuffer, myVkContext.graphicsQueue, myTransferSemaphores[0]);

	vkQueueWaitIdle(myVkContext.transferQueue);
	vkFreeCommandBuffers(myVkContext.device, myVkContext.myTransferCommandPool, 1, &commandBuffer);

	vkQueueWaitIdle(myVkContext.graphicsQueue);
	vkFreeCommandBuffers(myVkContext.device, myVkContext.myCommandPools[0], 1, &graphicsCommandBuffer);
}

void AM_VkRenderCore::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, uint32_t aLayerCount, VkCommandBuffer aCommandBuffer)
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
	barrier.subresourceRange.layerCount = aLayerCount;

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
	vkGetPhysicalDeviceFormatProperties(myVkContext.physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("texture image format does not support linear blitting!");

	// It should be noted that it is uncommon in practice to generate the mipmap levels at runtime anyway. 
	// Usually they are pre-generated and stored in the texture file alongside the base level to improve loading speed. 
	// Implementing resizing in software and loading multiple levels from a file is left as an exercise to the reader.

	VkCommandBuffer commandBuffer;
	BeginOneTimeCommands(commandBuffer, myVkContext.myCommandPools[0]);

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

	EndOneTimeCommands(commandBuffer, myVkContext.graphicsQueue, myVkContext.myCommandPools[0]);
}

void AM_VkRenderCore::CreateFilledStagingBuffer(TempBuffer& outBuffer, uint64_t aBufferSize, uint64_t aStrideSize, std::vector<void*>& someSources)
{
	VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingBufferInfo.size = aBufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VmaAllocationInfo allocationInfo;
	VkResult result = vmaCreateBuffer(myVMA, &stagingBufferInfo, &stagingAllocInfo, &outBuffer.myBuffer, &outBuffer.myAllocation, &allocationInfo);
	assert(result == VK_SUCCESS && "failed to create staging buffer!");

	uint64_t sourceCount = static_cast<uint64_t>(someSources.size());
	void* mappedData;
	vmaMapMemory(myVMA, outBuffer.myAllocation, &mappedData);
	for (uint64_t i = 0; i < sourceCount; ++i)
	{
		void* dst = (char*)mappedData + i * aStrideSize;
		memcpy(dst, someSources[i], aStrideSize);
	}
	vmaUnmapMemory(myVMA, outBuffer.myAllocation);
}

void AM_VkRenderCore::UploadToBuffer(uint64_t aBufferSize, void* aSource, const TempBuffer* aBuffer)
{
	VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingBufferInfo.size = aBufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo allocationInfo;
	VkResult result = vmaCreateBuffer(myVMA, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &allocationInfo);
	assert(result == VK_SUCCESS && "failed to create staging buffer!");

	vmaCopyMemoryToAllocation(myVMA, aSource, stagingAllocation, allocationInfo.offset, aBufferSize);
	CopyBuffer(stagingBuffer, &allocationInfo, aBuffer);
	vmaDestroyBuffer(myVMA, stagingBuffer, stagingAllocation);
}

void AM_VkRenderCore::CreateVertexBuffer(AM_Entity& outEntity, std::vector<Vertex>& someVertices)
{
	VkDeviceSize bufferSize = sizeof(Vertex) * someVertices.size();
	VkBufferCreateInfo vertexBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	vertexBufferCreateInfo.size = bufferSize;
	vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	VmaAllocationCreateInfo vertexBufferAllocInfo = {};
	vertexBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	TempBuffer vertexBuffer;
	VkResult result = vmaCreateBuffer(myVMA, &vertexBufferCreateInfo, &vertexBufferAllocInfo, &vertexBuffer.myBuffer, &vertexBuffer.myAllocation, nullptr);
	assert(result == VK_SUCCESS && "failed to create vertex buffer!");

	UploadToBuffer(bufferSize, (void*)someVertices.data(), &vertexBuffer);
	outEntity.SetVertexBuffer(vertexBuffer);
}

void AM_VkRenderCore::CreateIndexBuffer(AM_Entity& outEntity, std::vector<uint32_t>& someIndices)
{
	VkDeviceSize bufferSize = sizeof(uint32_t) * someIndices.size();
	VkBufferCreateInfo indexBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	indexBufferCreateInfo.size = bufferSize;
	indexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	VmaAllocationCreateInfo indexBufferAllocInfo = {};
	indexBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	TempBuffer indexBuffer;
	VkResult result = vmaCreateBuffer(myVMA, &indexBufferCreateInfo, &indexBufferAllocInfo, &indexBuffer.myBuffer, &indexBuffer.myAllocation, nullptr);
	assert(result == VK_SUCCESS && "failed to create vertex buffer!");
	
	UploadToBuffer(bufferSize, (void*)someIndices.data(), &indexBuffer);
	outEntity.SetIndexBuffer(indexBuffer);
}

void AM_VkRenderCore::AllocatePerEntityUBO(AM_Entity& outEntity)
{
	static constexpr uint64_t bufferSize = AM_VkRenderCoreConstants::UBO_ALIGNMENT * AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT;

	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	TempBuffer uniformBuffer;
	VmaAllocationInfo allocationInfo;
	VkResult result = vmaCreateBuffer(myVMA, &bufferInfo, &allocInfo, &uniformBuffer.myBuffer, &uniformBuffer.myAllocation, &allocationInfo);
	outEntity.SetUniformBuffer(uniformBuffer);

	assert(result == VK_SUCCESS && "failed to create uniform buffer!");
	assert(allocationInfo.pMappedData != nullptr && "Uniform buffer is not mapped!");
}

void AM_VkRenderCore::UpdateUniformBuffer(uint32_t currentImage, const AM_Camera& aCamera, std::unordered_map<uint64_t, AM_Entity>& someEntites, float aDeltaTime)
{
	UniformBufferObject ubo{};
	ubo.view = aCamera.GetViewMatrix();
	ubo.projection = aCamera.GetProjectionMatrix();
	ubo.inverseView = aCamera.GetInverseViewMatrix();
	ubo.ambientColor = { 1.f, 1.f, 1.f, 0.03f };

	int lightIndex = 0;
	for (auto& kv : someEntites)
	{
		auto& entity = kv.second;
		if (!entity.IsEmissive())
			continue;

		ubo.pointLightData[lightIndex].position = glm::vec4(entity.myTranslation, 1.f);
		ubo.pointLightData[lightIndex].color = glm::vec4(entity.GetColor(), entity.GetLightIntensity());
		++lightIndex;
	}
	ubo.numLights = lightIndex;
	ubo.deltaTime = aDeltaTime;

	static_assert(sizeof(ubo) <= AM_VkRenderCoreConstants::UBO_ALIGNMENT, "UBO size is larger than alignment!!!");
	vmaCopyMemoryToAllocation(myVMA, &ubo, myUniformBuffer.myAllocation, currentImage * AM_VkRenderCoreConstants::UBO_ALIGNMENT, sizeof(ubo));
}

void AM_VkRenderCore::BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = aCommandPool;
	allocInfo.commandBufferCount = 1;

	aCommandBuffer = myVkContext.AllocateCommandBuffer(allocInfo);

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

	vkFreeCommandBuffers(myVkContext.device, aCommandPool, 1, &commandBuffer);
}

void AM_VkRenderCore::CreateTextureSampler(VkSampler& outSampler, VkSamplerAddressMode anAddressMode, VkBorderColor aBorderColor, VkCompareOp aCompareOp)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = anAddressMode;
	samplerInfo.addressModeV = anAddressMode;
	samplerInfo.addressModeW = anAddressMode;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = myVkContext.deviceProperties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = aBorderColor;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = aCompareOp;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = static_cast<float>(myMipLevels); // or VK_LOD_CLAMP_NONE
	outSampler = myVkContext.CreateSampler(samplerInfo);
}

bool AM_VkRenderCore::HasStencilComponent(VkFormat format)
{
	return !((format ^ VK_FORMAT_D32_SFLOAT_S8_UINT) && (format ^ VK_FORMAT_D24_UNORM_S8_UINT));
}

void AM_VkRenderCore::LoadModel(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, const char* aFilePath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, aFilePath))
		throw std::runtime_error(warn + err);

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.myPosition =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			vertex.myNormal =
			{
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};
			vertex.texCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.f - attrib.texcoords[2 * index.texcoord_index + 1] // vulkan uv
			};
			vertex.myColor = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(outVertices.size());
				outVertices.push_back(vertex);
			}

			outIndices.push_back(uniqueVertices[vertex]);
		}
	}
}

void AM_VkRenderCore::LoadVertexData(AM_Entity& outEntity, const char* aFilePath)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	LoadModel(vertices, indices, aFilePath);
	CreateVertexBuffer(outEntity, vertices);
	CreateIndexBuffer(outEntity, indices);
}

// #FIX_ME: move out
void AM_VkRenderCore::LoadDefaultResources()
{
	AM_Entity* vikingRoomEntity = myEntityStorage->Add();
	LoadVertexData(*vikingRoomEntity, "../data/models/vikingroom.obj");
	vikingRoomEntity->myTranslation = { 12.f, 0.f, 0.f };
	AM_Texture& vikingRoomTexture = vikingRoomEntity->GetTexture();
	const char* textures[] = { AM_VkRenderCoreConstants::TEXTURE_PATH };
	CreateTextureImage(vikingRoomTexture.myImage, textures, 1);
	CreateImageView(vikingRoomTexture.myImageView, vikingRoomTexture.myImage.myImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1);
	CreateTextureSampler(vikingRoomTexture.mySampler, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_COMPARE_OP_ALWAYS);
	AllocatePerEntityUBO(*vikingRoomEntity);
	AllocatePerEntityDescriptorSets(*vikingRoomEntity);

	AM_Entity* vaseEntity = myEntityStorage->Add();
	LoadVertexData(*vaseEntity, "../data/models/smooth_vase.obj");
	vaseEntity->myTranslation = { -8.f, 0.f, 0.f };
	vaseEntity->myRotation = { 3.14159f, 0.f, 0.f };
	vaseEntity->myScale = { 20.f, 20.f, 20.f };
	AllocatePerEntityUBO(*vaseEntity);
	AllocatePerEntityDescriptorSets(*vaseEntity);

	AM_Entity* quadEntity = myEntityStorage->Add();
	LoadVertexData(*quadEntity, "../data/models/quad.obj");
	quadEntity->myTranslation = { 0.f, -1.f, 0.f };
	quadEntity->myScale = { 42.f, 1.f, 42.f };
	AllocatePerEntityUBO(*quadEntity);
	AllocatePerEntityDescriptorSets(*quadEntity);

	AM_Entity* skybox = myEntityStorage->Add();
	skybox->SetType(AM_Entity::SKYBOX);
	LoadVertexData(*skybox, "../data/models/cube.obj");
	skybox->myTranslation = { 0.f, 0.f, 0.f };
	skybox->myScale = { 1.f, 1.f, 1.f };
	skybox->SetIsSkybox(true);
	AM_Texture& skyboxTexture = skybox->GetTexture();
	CreateTextureImage(skyboxTexture.myImage, AM_VkRenderCoreConstants::CUBEMAP_TEXTURE_PATH, 6);
	CreateImageView(skyboxTexture.myImageView, skyboxTexture.myImage.myImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, 0, 6);
	CreateTextureSampler(skyboxTexture.mySampler, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_COMPARE_OP_NEVER);
	AllocatePerEntityUBO(*skybox);
	AllocatePerEntityDescriptorSets(*skybox);

	AM_Entity* pointLight1 = myEntityStorage->Add();
	pointLight1->SetType(AM_Entity::BILLBOARD);
	pointLight1->SetIsEmissive(true);
	pointLight1->myTranslation = { -5.f, 2.f, -.7f };
	pointLight1->SetColor({ 1.f, 0.1f, 0.1f });
	AllocatePerEntityUBO(*pointLight1);
	AllocatePerEntityDescriptorSets(*pointLight1);

	AM_Entity* pointLight2 = myEntityStorage->Add();
	pointLight1->SetType(AM_Entity::BILLBOARD);
	pointLight2->SetIsEmissive(true);
	pointLight2->myTranslation = { -5.f, 2.f, .7f };
	pointLight2->SetColor({ 1.f, 1.f, 0.1f });
	AllocatePerEntityUBO(*pointLight2);
	AllocatePerEntityDescriptorSets(*pointLight2);
}

void AM_VkRenderCore::Setup()
{
	// need a window layer in the future
	myWindowInstance.Init();

	if (!CheckExtensionSupport())
		throw std::runtime_error("extensions requested by GLFW, but not available!");

	if (!CheckInstanceLayerSupport())
		throw std::runtime_error("layers requested by application, but not available!");

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = ApplicationConstants::WINDOWNAME;
	appInfo.applicationVersion = ApplicationConstants::APP_VERSION;
	appInfo.pEngineName = ApplicationConstants::WINDOWNAME;
	appInfo.engineVersion = ApplicationConstants::APP_VERSION;
	appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 257);

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

	if (vkCreateInstance(&createInfo, nullptr, &myVkContext.instance) != VK_SUCCESS)
		throw std::runtime_error("failed to create Vulkan instance!");

	if (glfwCreateWindowSurface(myVkContext.instance, myWindowInstance.GetWindow(), nullptr, &myVkContext.surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");

	myVkContext.Init();

	VmaAllocatorCreateInfo vmaCreateInfo{};
	vmaCreateInfo.instance = myVkContext.instance;
	vmaCreateInfo.physicalDevice = myVkContext.physicalDevice;
	vmaCreateInfo.device = myVkContext.device;

	vmaCreateInfo.vulkanApiVersion = VK_MAKE_API_VERSION(0, 1, 3, 257);
	static VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	vmaCreateInfo.pVulkanFunctions = &vulkanFunctions;
	vmaCreateInfo.flags = 0;
	vmaCreateAllocator(&vmaCreateInfo, &myVMA);

	// numbers are arbitrary
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
	descriptorPoolSizes.resize(7);
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 4;
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 4;
	descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[2].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 2;
	descriptorPoolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	descriptorPoolSizes[3].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 4;
	descriptorPoolSizes[4].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	descriptorPoolSizes[4].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 4;
	descriptorPoolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolSizes[5].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 2;
	descriptorPoolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	descriptorPoolSizes[6].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 2;

	// maxSet = total set count * MAX_FRAMES_IN_FLIGHT
	myGlobalDescriptorPool = myVkContext.CreateDescriptorPool(static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 3, 0, descriptorPoolSizes);

	myTransferSemaphores.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	for (VkSemaphore& semaphore : myTransferSemaphores)
		semaphore = myVkContext.CreateSemaphore();

	myRenderContext = new AM_VkRenderContext(myVkContext, myWindowInstance, myVMA);


	auto bindingDesc = Vertex::GetBindingDescription();
	auto attriDesc = Vertex::GetAttributeDescriptions();
	auto bindingDesc2 = Particle::GetBindingDescription();
	auto attriDesc2 = Particle::GetAttributeDescriptions();

	myMeshRenderMethod = new AM_VkRenderMethodMesh(
		myVkContext, 
		myRenderContext->GetRenderPass(),
		"../data/shader_bytecode/shader.vert.spv",
		"../data/shader_bytecode/shader.frag.spv",
		1,
		static_cast<uint32_t>(attriDesc.size()),
		&bindingDesc,
		attriDesc.data());

	myBillboardRenderMethod = new AM_VkRenderMethodBillboard(
		myVkContext, 
		myRenderContext->GetRenderPass(),
		"../data/shader_bytecode/pointlight.vert.spv",
		"../data/shader_bytecode/pointlight.frag.spv", 
		0,
		0);

	myCubeMapRenderMethod = new AM_VkRenderMethodCubeMap(
		myVkContext, 
		myRenderContext->GetRenderPass(),
		"../data/shader_bytecode/skybox.vert.spv",
		"../data/shader_bytecode/skybox.frag.spv", 
		1,
		static_cast<uint32_t>(attriDesc.size()),
		&bindingDesc,
		attriDesc.data());

	LoadDefaultResources();
}

void AM_VkRenderCore::MainLoop()
{
	AM_Camera camera;
	camera.myTransformComp.myTranslation = { 0.f, 15.f, 35.f };
	camera.myTransformComp.myRotation = { 0.f, 0.f, 0.f };
	camera.SetPerspectiveProjection(0.7854f, myRenderContext->GetAspectRatio(), 0.1f, 100.f);
	camera.SetRotation(camera.myTransformComp.myTranslation, camera.myTransformComp.myRotation);

	while (!myWindowInstance.ShouldCloseWindow())
	{
		glfwPollEvents();
		float deltaTime = AM_SimpleTimer::GetInstance().GetDeltaTime();
		if (auto commandBufer = myRenderContext->BeginFrame())
		{
			UpdateCameraTransform(deltaTime, camera);
			UpdateUniformBuffer(myRenderContext->GetFrameIndex(), camera, myEntities, deltaTime);
			
			myRenderContext->BeginRenderPass(commandBufer);

			myCubeMapRenderMethod->Render(commandBufer, myCubeMapDescriptorSets[myRenderContext->GetFrameIndex()], myEntities, camera);
			myMeshRenderMethod->Render(commandBufer, myDescriptorSets[myRenderContext->GetFrameIndex()], myEntities, camera);
			myBillboardRenderMethod->Render(commandBufer, myDescriptorSets[myRenderContext->GetFrameIndex()], myEntities, camera);

			myRenderContext->EndRenderPass(commandBufer);
			myRenderContext->EndFrame();

			if (myWindowInstance.ShouldUpdateCamera())
			{
				camera.SetPerspectiveProjection(0.7854f, myRenderContext->GetAspectRatio(), 0.1f, 100.f);
				myWindowInstance.ResetCameraUpdateFlag();
			}
		}
	}

	vkDeviceWaitIdle(myVkContext.device);
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
