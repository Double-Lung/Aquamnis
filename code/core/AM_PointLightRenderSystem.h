#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "AM_VkContext.h"
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
		myVkContext.DestroyPipeline(myGraphicsPipeline);
		myVkContext.DestroyDescriptorSetLayout(myDescriptorSetLayout);
	}

	void Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout; }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myDescriptorSetLayout; }

private:

	AM_PointLightRenderSystem(const AM_PointLightRenderSystem&) = delete;
	AM_PointLightRenderSystem& operator=(const AM_PointLightRenderSystem&) = delete;

	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	// temp utils
	static std::vector<char> ReadFile(const std::string& filename);

	// should go into pipeline
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateDescriptorSetLayout();

	AM_VkContext& myVkContext;
	VkPipeline myGraphicsPipeline;
	VkPipelineLayout myPipelineLayout;

	// for pipeline and descriptor set
	VkDescriptorSetLayout myDescriptorSetLayout;
};

