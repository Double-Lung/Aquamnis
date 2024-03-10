#include "AM_RenderUtils.h"

#include <cassert>
#include <fstream>

void AM_RenderUtils::ReadFile(std::vector<char>& outShaderCode, const std::string& aFilename)
{
	std::ifstream file(aFilename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	size_t fileSize = static_cast<size_t>(file.tellg());
	outShaderCode.resize(fileSize);
	file.seekg(0);
	file.read(outShaderCode.data(), fileSize);
	file.close();
}

VkImageView AM_RenderUtils::CreateImageView(AM_VkContext& aVkContext, VkImage anImage, VkFormat aFormat, VkImageViewType aViewType, VkImageAspectFlags someAspectFlags, uint32_t aMipLevels, uint32_t aLayerCount)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = anImage;
	viewInfo.viewType = aViewType;
	viewInfo.format = aFormat;
	viewInfo.subresourceRange.aspectMask = someAspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = aMipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = aLayerCount;

	return aVkContext.CreateImageView(viewInfo);
}

VkSampler AM_RenderUtils::CreateTextureSampler(AM_VkContext& myVkContext, VkSamplerAddressMode anAddressMode, VkBorderColor aBorderColor, VkCompareOp aCompareOp, uint32_t aMipLevels)
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
	samplerInfo.maxLod = static_cast<float>(aMipLevels); // or VK_LOD_CLAMP_NONE

	return myVkContext.CreateSampler(samplerInfo);
}

VkCommandBuffer AM_RenderUtils::BeginOneTimeCommands(AM_VkContext& aVkContext, VkCommandPool& aCommandPool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = aCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	commandBuffer = aVkContext.AllocateCommandBuffer(allocInfo);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void AM_RenderUtils::EndOneTimeCommands(AM_VkContext& aVkContext, VkCommandBuffer aCommandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool)
{
	vkEndCommandBuffer(aCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &aCommandBuffer;

	vkQueueSubmit(aVkQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(aVkQueue);

	vkFreeCommandBuffers(aVkContext.device, aCommandPool, 1, &aCommandBuffer);
}

bool AM_RenderUtils::HasStencilComponent(VkFormat format)
{
	return !((format ^ VK_FORMAT_D32_SFLOAT_S8_UINT) && (format ^ VK_FORMAT_D24_UNORM_S8_UINT));
}

void AM_RenderUtils::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, uint32_t aLayerCount, VkCommandBuffer aCommandBuffer)
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

void AM_RenderUtils::check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}
