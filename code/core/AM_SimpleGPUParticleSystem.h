#pragma once
#include "AM_VkContext.h"
#include "AM_VkPipeline.h"
#include "TempBuffer.h"
#include <glm/glm.hpp>
#include <unordered_map>

class AM_SimpleGPUParticleSystem
{
public:
	AM_SimpleGPUParticleSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass);
	~AM_SimpleGPUParticleSystem()
	{
		myVkContext.DestroyPipelineLayout(myPipelineLayout);
		myVkContext.DestroyPipelineLayout(myGfxPipelineLayout);
		myVkContext.DestroyDescriptorSetLayout(myDescriptorSetLayout);
		myVkContext.DestroyDescriptorSetLayout(myGfxDescriptorSetLayout);
	}

	AM_SimpleGPUParticleSystem(const AM_SimpleGPUParticleSystem&) = delete;
	AM_SimpleGPUParticleSystem& operator=(const AM_SimpleGPUParticleSystem&) = delete;

	void Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, const TempBuffer* anSSBO);
	// need to run outside of any render pass
	void DispatchWork(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet);
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout; }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myDescriptorSetLayout; }

private:
	void CreateComputePipeline();
	void CreateGraphicsPipeline(VkRenderPass aRenderPass);

	AM_VkContext& myVkContext;
	AM_VkPipeline myComputePipeline;
	VkPipelineLayout myPipelineLayout;
	VkDescriptorSetLayout myDescriptorSetLayout;

	AM_VkPipeline myGraphicsPipeline;
	VkPipelineLayout myGfxPipelineLayout;
	VkDescriptorSetLayout myGfxDescriptorSetLayout;
	
	uint32_t* myMaxComputeWorkGroupCount;
	uint32_t myMaxComputeWorkGroupInvocations;
	uint32_t* myMaxComputeWorkGroupSize;
};

