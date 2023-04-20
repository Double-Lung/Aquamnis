#pragma once
#include "AM_VkContext.h"
#include "AM_VkPrimitives.h"

class AM_Buffer;
class AM_Entity;
class AM_SimpleRenderSystem
{
public:
	AM_SimpleRenderSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);
	~AM_SimpleRenderSystem(){}

	void RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::vector<AM_Entity>& someEntites);

private:
	struct PushConstantData
	{
		glm::vec3 offset;
		glm::mat4 transform;
	};

	AM_SimpleRenderSystem(const AM_SimpleRenderSystem&) = delete;
	AM_SimpleRenderSystem& operator=(const AM_SimpleRenderSystem&) = delete;

	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	// temp utils
	static std::vector<char> ReadFile(const std::string& filename);
	static float GetElapsedTimeInSeconds();

	// should go into pipeline
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateDescriptorSetLayout();

	AM_VkContext& myVkContext;
	AM_VkPipeline myGraphicsPipeline;
	AM_VkPipelineLayout myPipelineLayout;

	// for pipeline and descriptor set
	AM_VkDescriptorSetLayout myDescriptorSetLayout;
};

