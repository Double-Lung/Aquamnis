#pragma once
#include "AM_VkContext.h"
#include "AM_VkPipeline.h"
#include <unordered_map>
#include <glm/glm.hpp>

class AM_Entity;
class AM_Camera;
class AM_SimpleRenderSystem
{
public:
	AM_SimpleRenderSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);
	~AM_SimpleRenderSystem()
	{
		myVkContext.DestroyPipelineLayout(myPipelineLayout);
		myVkContext.DestroyDescriptorSetLayout(myDescriptorSetLayout);
	}
	AM_SimpleRenderSystem(const AM_SimpleRenderSystem&) = delete;
	AM_SimpleRenderSystem& operator=(const AM_SimpleRenderSystem&) = delete;

	void RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout; }
	VkDescriptorSetLayout GetDescriptorSetLayout() { return myDescriptorSetLayout; }

private:
	struct PushConstantData
	{
		glm::mat4 normalMat;
		glm::mat4 transform;
	};

	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	AM_VkContext& myVkContext;
	AM_VkPipeline myGraphicsPipeline;
	VkPipelineLayout myPipelineLayout;
	VkDescriptorSetLayout myDescriptorSetLayout;
};

