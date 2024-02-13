#pragma once
#include "AM_VkContext.h"
#include "AM_VkPipeline.h"
#include <unordered_map>
#include <glm/glm.hpp>

class AM_Entity;
class AM_Camera;
class AM_PointLightRenderSystem
{
public:
	AM_PointLightRenderSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);
	AM_PointLightRenderSystem(const AM_PointLightRenderSystem&) = delete;
	AM_PointLightRenderSystem& operator=(const AM_PointLightRenderSystem&) = delete;

	void RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);
	VkDescriptorSetLayout GetDescriptorSetLayout() { return myGraphicsPipeline.GetDescriptorSetLayout(); }

private:
	struct PointLightPushConstants
	{
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	AM_VkContext& myVkContext;
	AM_VkPipeline myGraphicsPipeline;
};

