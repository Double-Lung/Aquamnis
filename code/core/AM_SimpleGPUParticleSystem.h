#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "AM_VkContext.h"
#include <unordered_map>
#include "TempBuffer.h"
#include <glm/glm.hpp>

class AM_SimpleGPUParticleSystem
{
public:
	AM_SimpleGPUParticleSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);
	~AM_SimpleGPUParticleSystem()
	{
		myVkContext.DestroyPipelineLayout(myPipelineLayout);
		myVkContext.DestroyPipelineLayout(myGfxPipelineLayout);
		vkDestroyPipeline(myVkContext.device, myGraphicsPipeline, nullptr);
		vkDestroyPipeline(myVkContext.device, myComputePipeline, nullptr);
		myVkContext.DestroyDescriptorSetLayout(myDescriptorSetLayout);
		myVkContext.DestroyDescriptorSetLayout(myGfxDescriptorSetLayout);
	}

	void Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, const TempBuffer* anSSBO);
	// need to run outside of any render pass
	void DispatchWork(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout; }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myDescriptorSetLayout; }

private:
	struct PushConstantData
	{
		glm::mat4 normalMat;
		glm::mat4 transform;
	};

	AM_SimpleGPUParticleSystem(const AM_SimpleGPUParticleSystem&) = delete;
	AM_SimpleGPUParticleSystem& operator=(const AM_SimpleGPUParticleSystem&) = delete;

	void CreateComputePipeline();
	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	// should go into pipeline
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateDescriptorSetLayout();

	AM_VkContext& myVkContext;
	VkPipeline myComputePipeline;
	VkPipelineLayout myPipelineLayout;
	VkDescriptorSetLayout myDescriptorSetLayout;

	VkPipeline myGraphicsPipeline;
	VkPipelineLayout myGfxPipelineLayout;
	VkDescriptorSetLayout myGfxDescriptorSetLayout;
	
	uint32_t* myMaxComputeWorkGroupCount;
	uint32_t myMaxComputeWorkGroupInvocations;
	uint32_t* myMaxComputeWorkGroupSize;
};

