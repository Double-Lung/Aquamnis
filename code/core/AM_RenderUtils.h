#pragma once
#include "AM_VkContext.h"
#include <vector>

struct TempImage;
VK_DEFINE_HANDLE(VmaAllocator);
namespace AM_RenderUtils
{
	void ReadFile(std::vector<char>& outShaderCode, const std::string& aFilename);
	VkImageView CreateImageView(AM_VkContext& aVkContext, VkImage anImage, VkFormat aFormat, VkImageViewType aViewType, VkImageAspectFlags someAspectFlags, uint32_t aMipLevels, uint32_t aLayerCount);
	VkSampler CreateTextureSampler(AM_VkContext& myVkContext, VkSamplerAddressMode anAddressMode, VkBorderColor aBorderColor, VkCompareOp aCompareOp, uint32_t aMipLevels = 1);
	VkCommandBuffer BeginOneTimeCommands(AM_VkContext& aVkContext, VkCommandPool& aCommandPool);
	void EndOneTimeCommands(AM_VkContext& aVkContext, VkCommandBuffer aCommandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool);

	bool HasStencilComponent(VkFormat format);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, uint32_t aLayerCount, VkCommandBuffer aCommandBuffer);
}
