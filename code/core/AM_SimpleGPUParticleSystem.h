#pragma once
#include "AM_VkContext.h"
#include "AM_VkPipeline.h"
#include "TempBuffer.h"
#include <glm/glm.hpp>
#include <unordered_map>

class AM_SimpleGPUParticleSystem
{
public:
	AM_SimpleGPUParticleSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);
	AM_SimpleGPUParticleSystem(const AM_SimpleGPUParticleSystem&) = delete;
	AM_SimpleGPUParticleSystem& operator=(const AM_SimpleGPUParticleSystem&) = delete;

	void Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, const TempBuffer* anSSBO);
	VkDescriptorSetLayout GetDescriptorSetLayout() { return myGraphicsPipeline.GetDescriptorSetLayout(); }

private:
	void CreateGraphicsPipeline(VkRenderPass aRenderPass);
	AM_VkContext& myVkContext;
	AM_VkPipeline myGraphicsPipeline;
};

