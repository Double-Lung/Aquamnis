#pragma once
#include "AM_Camera.h"
#include "AM_EntityStorage.h"
#include <vulkan/vulkan.h>

struct AM_FrameRenderInfo
{
	VkCommandBuffer myCommandBuffer{nullptr};
	VkDescriptorSet myGlobalDescriptorSet{nullptr};
	AM_EntityStorage* myEntityStorage{nullptr};
	AM_Camera* myCamera{nullptr};
	uint32_t myFrameIndex{0};
};