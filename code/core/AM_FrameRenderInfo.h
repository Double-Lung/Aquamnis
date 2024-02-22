#pragma once
#include <vulkan/vulkan.h>
class AM_Camera;
class AM_EntityStorage;
struct AM_FrameRenderInfo
{
	VkCommandBuffer myCommandBuffer{nullptr};
	VkDescriptorSet myGlobalDescriptorSet{nullptr};
	AM_EntityStorage* myEntityStorage{nullptr};
	AM_Camera* myCamera{nullptr};
	uint32_t myFrameIndex{0};
};