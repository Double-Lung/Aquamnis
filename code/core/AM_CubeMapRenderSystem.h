#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
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
	~AM_CubeMapRenderSystem()
	{
		myVkContext.DestroyPipelineLayout(myPipelineLayout);
		myVkContext.DestroyDescriptorSetLayout(myDescriptorSetLayout);
	}
	AM_CubeMapRenderSystem(const AM_CubeMapRenderSystem&) = delete;
	AM_CubeMapRenderSystem& operator=(const AM_CubeMapRenderSystem&) = delete;

	void RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout; }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myDescriptorSetLayout; }

private:
	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	AM_VkContext& myVkContext;
	AM_VkPipeline myGraphicsPipeline;
	VkPipelineLayout myPipelineLayout;
	VkDescriptorSetLayout myDescriptorSetLayout;
};

