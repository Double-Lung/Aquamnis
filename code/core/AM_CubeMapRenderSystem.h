#pragma once
#include "AM_VkContext.h"
#include "AM_VkPipeline.h"
#include <unordered_map>
#include <glm/glm.hpp>

class AM_Entity;
class AM_Camera;
class AM_CubeMapRenderSystem
{
public:
	AM_CubeMapRenderSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);

	AM_CubeMapRenderSystem(const AM_CubeMapRenderSystem&) = delete;
	AM_CubeMapRenderSystem& operator=(const AM_CubeMapRenderSystem&) = delete;

	void RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);
	VkDescriptorSetLayout GetDescriptorSetLayout() { return myGraphicsPipeline.GetDescriptorSetLayout(); }

private:
	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	AM_VkContext& myVkContext;
	AM_VkPipeline myGraphicsPipeline;
};

