#include "AM_ComputeParticle.h"
#include "AM_PipelineUtils.h"
#include "AM_VkDescriptorSetLayoutBuilder.h"

AM_ComputeParticle::AM_ComputeParticle(AM_VkContext& aVkContext, const std::string& aComputeShaderPath)
	: myVkContext(aVkContext)
	, myComputePipeline(aVkContext)
{
	VkComputePipelineCreateInfo pipelineInfo{};
	AM_PipelineUtils::SetDefaultComputeCreateInfo(pipelineInfo);

	AM_VkDescriptorSetLayoutBuilder builder;
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	builder.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);

	myComputePipeline.CreatePipeline(aComputeShaderPath, builder, pipelineInfo);
}

void AM_ComputeParticle::DispatchWork(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional
	if (vkBeginCommandBuffer(aCommandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording compute command buffer!");

	myComputePipeline.BindCompute(aCommandBuffer);
	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, myComputePipeline.GetPipelineLayout(), 0, 1, &aDescriptorSet, 0, nullptr);

	static constexpr int PARTICLE_COUNT = 2000;
	vkCmdDispatch(aCommandBuffer, (PARTICLE_COUNT - 1) / 256 + 1, 1, 1);

	if (vkEndCommandBuffer(aCommandBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record compute command buffer!");
}


