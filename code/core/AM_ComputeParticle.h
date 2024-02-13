#pragma once
#include "AM_VkContext.h"
#include "AM_VkPipeline.h"

class AM_ComputeParticle
{
public:
	AM_ComputeParticle(AM_VkContext& aVkContext);
	AM_ComputeParticle(const AM_ComputeParticle&) = delete;
	AM_ComputeParticle& operator=(const AM_ComputeParticle&) = delete;

	// need to run outside of any render pass
	void DispatchWork(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet);
	VkDescriptorSetLayout GetDescriptorSetLayout() { return myComputePipeline.GetDescriptorSetLayout(); }
private:
	void CreateComputePipeline();

	AM_VkContext& myVkContext;
	AM_VkPipeline myComputePipeline;
};