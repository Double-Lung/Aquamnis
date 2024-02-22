#include "AM_VkRenderCore.h"

#include "AM_Camera.h"
#include "AM_Entity.h"
#include "AM_EntityStorage.h"
#include "AM_FrameRenderInfo.h"
#include "AM_RenderUtils.h"
#include "AM_Texture.h"
#include "AM_TempScene.h"
#include "AM_VkDescriptorSetWritesBuilder.h"
#include "AM_VkRenderContext.h"
#include "AM_VkRenderCoreConstants.h"
#include "AM_VkRenderMethodBillboard.h"
#include "AM_VkRenderMethodCubeMap.h"
#include "AM_VkRenderMethodMesh.h"
#include "AM_VmaUsage.h"
#include "AM_Window.h"
#include "TempBuffer.h"
#include "TempImage.h"
#include <stb_image.h>
#include <tiny_obj_loader.h>

AM_VkRenderCore::AM_VkRenderCore(AM_Window& aWindowInstance)
	: myWindowInstance(aWindowInstance)
	, myVkContext{}
	, myDefaultTexture{}
	, myGlobalDescriptorPool{nullptr}
	, myVMA{nullptr}
	, myRenderContext{ nullptr }
	, myMeshRenderMethod{ nullptr }
	, myBillboardRenderMethod{ nullptr }
	, myCubeMapRenderMethod{ nullptr }
	, myMipLevels(1)
{
}

AM_VkRenderCore::~AM_VkRenderCore()
{
	delete myCubeMapRenderMethod;
	delete myBillboardRenderMethod;
	delete myMeshRenderMethod;
	delete myRenderContext;

	vmaDestroyImage(myVMA, myDefaultTexture.myImage.myImage, myDefaultTexture.myImage.myAllocation);
	myVkContext.DestroyImageView(myDefaultTexture.myImageView);
	myVkContext.DestroySampler(myDefaultTexture.mySampler);

	vmaDestroyAllocator(myVMA);
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

void AM_VkRenderCore::CreateTextureImage(TempImage& outImage, const char** somePaths, uint32_t aLayerCount, VkImageCreateFlags someFlags)
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
	uint64_t stride = static_cast<uint64_t>(texWidth) * static_cast<uint64_t>(texHeight) * static_cast<uint64_t>(STBI_rgb_alpha);
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
	imageInfo.arrayLayers = aLayerCount;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = someFlags;

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkResult result = vmaCreateImage(myVMA, &imageInfo, &allocationInfo, &outImage.myImage, &outImage.myAllocation, nullptr);
	assert(result == VK_SUCCESS && "failed to create image!");

	VkCommandBuffer commandBuffer = AM_RenderUtils::BeginOneTimeCommands(myVkContext, myVkContext.myTransferCommandPool);
	AM_RenderUtils::TransitionImageLayout(outImage.myImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, myMipLevels, aLayerCount, commandBuffer);
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
	postCopyTransferMemoryBarrier.subresourceRange.layerCount = aLayerCount;
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

	VkCommandBuffer graphicsCommandBuffer = AM_RenderUtils::BeginOneTimeCommands(myVkContext, myVkContext.myCommandPools[0]);

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
	postCopyGraphicsMemoryBarrier.subresourceRange.layerCount = aLayerCount;
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
	VkCommandBuffer commandBuffer = AM_RenderUtils::BeginOneTimeCommands(myVkContext, myVkContext.myTransferCommandPool);

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

	VkCommandBuffer graphicsCommandBuffer = AM_RenderUtils::BeginOneTimeCommands(myVkContext, myVkContext.myCommandPools[0]);

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

	VkCommandBuffer commandBuffer = AM_RenderUtils::BeginOneTimeCommands(myVkContext, myVkContext.myCommandPools[0]);

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

	AM_RenderUtils::EndOneTimeCommands(myVkContext, commandBuffer, myVkContext.graphicsQueue, myVkContext.myCommandPools[0]);
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
	outEntity.SetIndexBufferSize(static_cast<uint32_t>(someIndices.size()));
}

VkDescriptorSetLayout AM_VkRenderCore::GePerEntitytDescriptorSetLayout(const AM_Entity& anEntity)
{
	switch (anEntity.GetType())
	{
		case AM_Entity::MESH:
			return myMeshRenderMethod->GetDescriptorSetLayout();
		case AM_Entity::BILLBOARD:
			return myBillboardRenderMethod->GetDescriptorSetLayout();
		case AM_Entity::SKYBOX:
			return myCubeMapRenderMethod->GetDescriptorSetLayout();
		default:
			assert(false && "Not implemented");
			return nullptr;
	}
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

void AM_VkRenderCore::AllocatePerEntityDescriptorSets(AM_Entity& outEntity)
{
	std::vector<VkDescriptorSetLayout> layouts(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT, GePerEntitytDescriptorSetLayout(outEntity));
	std::vector<VkDescriptorSet>& descriptorSets = outEntity.GetDescriptorSets();
	descriptorSets.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myVkContext.AllocateDescriptorSets(myGlobalDescriptorPool, layouts, descriptorSets);

	for (int i = 0; i < AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		AM_VkDescriptorSetWritesBuilder builder{ myGlobalDescriptorPool };

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = outEntity.GetUniformBuffer()->myBuffer;
		bufferInfo.offset = i * AM_VkRenderCoreConstants::UBO_ALIGNMENT;
		bufferInfo.range = sizeof(AM_Entity::EntityUBO); // or VK_WHOLE_SIZE

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = outEntity.GetTexture().myImageView;
		imageInfo.sampler = outEntity.GetTexture().mySampler;

		switch (outEntity.GetType())
		{
		case AM_Entity::MESH:
		{
			if (bufferInfo.buffer)
				builder.WriteBuffer(0, &bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			if (imageInfo.imageView)
				builder.WriteImage(1, &imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			break;
		}
		case AM_Entity::SKINNEDMESH:
			assert(false && "Not implemented");
		case AM_Entity::PARTICLE:
			assert(false && "Not implemented");
		case AM_Entity::BILLBOARD:
		{
			if (bufferInfo.buffer)
				builder.WriteBuffer(0, &bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			break;
		}
		case AM_Entity::SKYBOX:
			if (imageInfo.imageView)
				builder.WriteImage(0, &imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			break;
		case AM_Entity::POINT:
			assert(false && "Not implemented");
		case AM_Entity::LINE:
			assert(false && "Not implemented");
		}

		myVkContext.WriteToDescriptorSet(descriptorSets[i], builder.GetWrites());
	}
}

void AM_VkRenderCore::WriteEntityUniformBuffer(AM_Entity& anEntity)
{
	if (!anEntity.GetUniformBuffer()->myBuffer)
		return;

	uint32_t frameIdx = myRenderContext->GetFrameIndex();
	if (!anEntity.GetShouldUpdateUniformBuffer(frameIdx))
		return;

	AM_Entity::EntityUBO& ubo = anEntity.GetUBO();
	static_assert(sizeof(ubo) <= AM_VkRenderCoreConstants::UBO_ALIGNMENT, "UBO size is larger than alignment!!!");
	vmaCopyMemoryToAllocation(myVMA, &ubo, anEntity.GetUniformBuffer()->myAllocation, frameIdx * AM_VkRenderCoreConstants::UBO_ALIGNMENT, sizeof(ubo));

	anEntity.ResetUpdateFlag(frameIdx);
}

void AM_VkRenderCore::WriteSceneUbiformBuffer(AM_TempScene& aScene)
{
	uint32_t frameIdx = myRenderContext->GetFrameIndex();
	if (!aScene.GetShouldUpdateUniformBuffer(frameIdx))
		return;
	
	GlobalUBO& ubo = aScene.GetUBO();
	static_assert(sizeof(ubo) <= AM_VkRenderCoreConstants::UBO_ALIGNMENT, "UBO size is larger than alignment!!!");
	vmaCopyMemoryToAllocation(myVMA, &ubo, aScene.GetUniformBuffer()->myAllocation, frameIdx * AM_VkRenderCoreConstants::UBO_ALIGNMENT, sizeof(ubo));

	aScene.ResetUpdateFlag(frameIdx);
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

void AM_VkRenderCore::CreateRenderMethods(VkDescriptorSetLayout aGlobalLayout)
{
	auto bindingDesc = Vertex::GetBindingDescription();
	auto attriDesc = Vertex::GetAttributeDescriptions();
	auto bindingDesc2 = Particle::GetBindingDescription();
	auto attriDesc2 = Particle::GetAttributeDescriptions();

	myMeshRenderMethod = new AM_VkRenderMethodMesh(
		myVkContext,
		myRenderContext->GetRenderPass(),
		"../data/shader_bytecode/shader.vert.spv",
		"../data/shader_bytecode/shader.frag.spv",
		aGlobalLayout,
		1,
		static_cast<uint32_t>(attriDesc.size()),
		&bindingDesc,
		attriDesc.data());

	myBillboardRenderMethod = new AM_VkRenderMethodBillboard(
		myVkContext,
		myRenderContext->GetRenderPass(),
		"../data/shader_bytecode/pointlight.vert.spv",
		"../data/shader_bytecode/pointlight.frag.spv",
		aGlobalLayout,
		0,
		0);

	myCubeMapRenderMethod = new AM_VkRenderMethodCubeMap(
		myVkContext,
		myRenderContext->GetRenderPass(),
		"../data/shader_bytecode/skybox.vert.spv",
		"../data/shader_bytecode/skybox.frag.spv",
		aGlobalLayout,
		1,
		static_cast<uint32_t>(attriDesc.size()),
		&bindingDesc,
		attriDesc.data());
}

void AM_VkRenderCore::LoadDefaultTexture()
{
	const char* defaultTexture[] = { "../data/textures/default.png" };
	CreateTextureImage(myDefaultTexture.myImage, defaultTexture, 1);
	myDefaultTexture.myImageView = AM_RenderUtils::CreateImageView(myVkContext, myDefaultTexture.myImage.myImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, myMipLevels, 1);
	myDefaultTexture.mySampler = AM_RenderUtils::CreateTextureSampler(myVkContext, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_COMPARE_OP_ALWAYS, myMipLevels);
}

void AM_VkRenderCore::Setup()
{
	if (!CheckExtensionSupport())
		throw std::runtime_error("extensions requested by GLFW, but not available!");

	if (!CheckInstanceLayerSupport())
		throw std::runtime_error("layers requested by application, but not available!");

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "dummy";
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	appInfo.pEngineName = "dummy";
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
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
	descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 16;
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 16;
	descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[2].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 1;
	descriptorPoolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	descriptorPoolSizes[3].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 1;
	descriptorPoolSizes[4].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	descriptorPoolSizes[4].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 1;
	descriptorPoolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolSizes[5].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 1;
	descriptorPoolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	descriptorPoolSizes[6].descriptorCount = static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 1;

	// maxSet = total set count * MAX_FRAMES_IN_FLIGHT
	myGlobalDescriptorPool = myVkContext.CreateDescriptorPool(static_cast<uint32_t>(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT) * 16, 0, descriptorPoolSizes);

	myTransferSemaphores.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	for (VkSemaphore& semaphore : myTransferSemaphores)
		semaphore = myVkContext.CreateSemaphore();

	myRenderContext = new AM_VkRenderContext(myVkContext, myWindowInstance, myVMA);
	LoadDefaultTexture();
}

void AM_VkRenderCore::Render(AM_Camera& aCamera, AM_TempScene& aScene, AM_EntityStorage& anEntityStorage)
{
	if (auto commandBufer = myRenderContext->BeginFrame())
	{
		WriteSceneUbiformBuffer(aScene);
		for (auto& kv : anEntityStorage.GetStorage())
		{
			AM_Entity* entity = kv.second;
			WriteEntityUniformBuffer(*entity);
		}

		/*
		* compute work needs to be submitted before starting any render pass
		*/

		uint32_t frameIdx = myRenderContext->GetFrameIndex();
		AM_FrameRenderInfo info
		{
			commandBufer,
			aScene.GetDescriptorSets()[frameIdx],
			&anEntityStorage,
			&aCamera,
			frameIdx
		};

		myRenderContext->BeginRenderPass(commandBufer);

		std::vector<AM_Entity*> entities = { anEntityStorage.GetIfExist(aScene.GetSkyboxId()) };
		myCubeMapRenderMethod->Render(info, entities);

		anEntityStorage.GetEntitiesOfType(entities, AM_Entity::MESH);
		myMeshRenderMethod->Render(info, entities);

		anEntityStorage.GetEntitiesOfType(entities, AM_Entity::BILLBOARD);
		myBillboardRenderMethod->Render(info, entities);

		myRenderContext->EndRenderPass(commandBufer);
		myRenderContext->EndFrame();
	}
}

void AM_VkRenderCore::OnEnd()
{
	vkDeviceWaitIdle(myVkContext.device);
}

void AM_VkRenderCore::InitScene(AM_TempScene& aScene)
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
	aScene.SetUniformBuffer(uniformBuffer);

	assert(result == VK_SUCCESS && "failed to create uniform buffer!");
	assert(allocationInfo.pMappedData != nullptr && "Uniform buffer is not mapped!");

	// create descriptor set layout
	AM_VkDescriptorSetLayoutBuilder layoutBuilder;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	layoutBuilder.GetBindings(bindings);
	VkDescriptorSetLayout descriptorSetLayout = myVkContext.CreateDescriptorSetLayout(bindings);
	aScene.SetDescriptorSetLayout(descriptorSetLayout);

	// create descriptor sets
	std::vector<VkDescriptorSetLayout> layouts(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	std::vector<VkDescriptorSet>& globalDescriptorSets = aScene.GetDescriptorSets();
	globalDescriptorSets.resize(AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT);
	myVkContext.AllocateDescriptorSets(myGlobalDescriptorPool, layouts, globalDescriptorSets);

	for (int i = 0; i < AM_VkRenderCoreConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		AM_VkDescriptorSetWritesBuilder builder{ myGlobalDescriptorPool };

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer.myBuffer;
		bufferInfo.offset = i * AM_VkRenderCoreConstants::UBO_ALIGNMENT;
		bufferInfo.range = sizeof(GlobalUBO); // or VK_WHOLE_SIZE

		builder.WriteBuffer(0, &bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		myVkContext.WriteToDescriptorSet(globalDescriptorSets[i], builder.GetWrites());
	}

	CreateRenderMethods(descriptorSetLayout);
}

AM_Entity* AM_VkRenderCore::LoadSkybox(const char** someTexturePaths, AM_EntityStorage& anEntityStorage)
{
	AM_Entity* skybox = anEntityStorage.Add();
	skybox->SetType(AM_Entity::SKYBOX);
	skybox->SetIsSkybox(true);
	LoadVertexData(*skybox, "../data/models/cube.obj");
	AM_Texture& skyboxTexture = skybox->GetTexture();
	CreateTextureImage(skyboxTexture.myImage, someTexturePaths, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
	skyboxTexture.myImageView = AM_RenderUtils::CreateImageView(myVkContext, skyboxTexture.myImage.myImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, myMipLevels, 6);
	skyboxTexture.mySampler = AM_RenderUtils::CreateTextureSampler(myVkContext, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_COMPARE_OP_NEVER, myMipLevels);
	AllocatePerEntityDescriptorSets(*skybox);
	return skybox;
}

AM_Entity* AM_VkRenderCore::LoadEntity(const char** someTexturePaths, const char* aModelPath, AM_EntityStorage& anEntityStorage, uint8_t aType)
{
	AM_Entity* entity = anEntityStorage.Add();
	entity->SetType(AM_Entity::EntityType(aType));

	if (aModelPath)
		LoadVertexData(*entity, aModelPath);

	if (someTexturePaths)
	{
		AM_Texture& entityTexture = entity->GetTexture();
		CreateTextureImage(entityTexture.myImage, someTexturePaths, 1);
		entityTexture.myImageView = AM_RenderUtils::CreateImageView(myVkContext, entityTexture.myImage.myImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, myMipLevels, 1);
		entityTexture.mySampler = AM_RenderUtils::CreateTextureSampler(myVkContext, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_COMPARE_OP_ALWAYS, myMipLevels);
	}
	else
	{
		AM_Texture& entityTexture = entity->GetTexture();
		entityTexture = myDefaultTexture;
		entityTexture.myIsDefault = true;
	}

	AllocatePerEntityUBO(*entity);
	AllocatePerEntityDescriptorSets(*entity);

	return entity;
}

void AM_VkRenderCore::DestroyEntities(AM_EntityStorage& anEntityStorage)
{
	anEntityStorage.DestroyEntities(myVkContext, myVMA);
}

void AM_VkRenderCore::DestroyScene(AM_TempScene& aScene)
{
	myVkContext.DestroyDescriptorSetLayout(aScene.GetDescriptorSetLayout());
	if (auto* uniformBuffer = aScene.GetUniformBuffer())
		vmaDestroyBuffer(myVMA, uniformBuffer->myBuffer, uniformBuffer->myAllocation);
}
