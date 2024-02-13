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
	~AM_PointLightRenderSystem()
	{
		myVkContext.DestroyPipelineLayout(myPipelineLayout);
		myVkContext.DestroyDescriptorSetLayout(myDescriptorSetLayout);
	}
	AM_PointLightRenderSystem(const AM_PointLightRenderSystem&) = delete;
	AM_PointLightRenderSystem& operator=(const AM_PointLightRenderSystem&) = delete;

	void Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout; }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myDescriptorSetLayout; }

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
	VkPipelineLayout myPipelineLayout;
	VkDescriptorSetLayout myDescriptorSetLayout;
};

