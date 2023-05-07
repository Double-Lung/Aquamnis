#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "AM_VkContext.h"
#include "AM_VkPrimitives.h"
#include "AM_VkDescriptorUtils.h"
#include <glm/glm.hpp>

class AM_Buffer;
class AM_Entity;
class AM_Camera;
class AM_SimpleRenderSystem
{
public:
	AM_SimpleRenderSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);
	~AM_SimpleRenderSystem(){}

	void RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::vector<AM_Entity>& someEntites, const AM_Camera& aCamera);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout.GetDescriptorSetLayout(); }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myDescriptorSetLayout.GetDescriptorSetLayout(); }
	AM_VkDescriptorSetLayout& GetDescriptorSetLayoutWrapper() { return myDescriptorSetLayout; }

private:
	struct PushConstantData
	{
		glm::mat4 normalMat;
		glm::mat4 transform;
	};

	AM_SimpleRenderSystem(const AM_SimpleRenderSystem&) = delete;
	AM_SimpleRenderSystem& operator=(const AM_SimpleRenderSystem&) = delete;

	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	// temp utils
	static std::vector<char> ReadFile(const std::string& filename);

	// should go into pipeline
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateDescriptorSetLayout();

	AM_VkContext& myVkContext;
	AM_VkPipeline myGraphicsPipeline;
	AM_VkPipelineLayout myPipelineLayout;

	// for pipeline and descriptor set
	AM_VkDescriptorSetLayout myDescriptorSetLayout;
};

