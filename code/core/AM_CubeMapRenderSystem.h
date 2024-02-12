#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "AM_VkContext.h"
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
		myVkContext.DestroyPipeline(myGraphicsPipeline);
		myVkContext.DestroyDescriptorSetLayout(myDescriptorSetLayout);
	}

	void RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout; }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myDescriptorSetLayout; }

private:
	struct PushConstantData
	{
		glm::mat4 normalMat;
		glm::mat4 transform;
	};

	AM_CubeMapRenderSystem(const AM_CubeMapRenderSystem&) = delete;
	AM_CubeMapRenderSystem& operator=(const AM_CubeMapRenderSystem&) = delete;

	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	// #FIX_ME temp utils
	static std::vector<char> ReadFile(const std::string& filename);

	// #FIX_ME should go into pipeline
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateDescriptorSetLayout();

	AM_VkContext& myVkContext;
	VkPipeline myGraphicsPipeline;
	VkPipelineLayout myPipelineLayout;

	// for pipeline and descriptor set
	VkDescriptorSetLayout myDescriptorSetLayout;
};

