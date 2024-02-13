#include "AM_SimpleGPUParticleSystem.h"
#include "AM_Entity.h"
#include "AM_Camera.h"
#include "AM_SimpleTimer.h"
#include "AM_Particle.h"
#include "AM_VkDescriptorSetLayoutBuilder.h"
#include "AM_RenderUtils.h"
#include "AM_PipelineUtils.h"
#include "vk_mem_alloc.h"
#include <array>
#include <algorithm>

AM_SimpleGPUParticleSystem::AM_SimpleGPUParticleSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass)
	: myVkContext{ aVkContext }
	, myGraphicsPipeline{aVkContext}
{
	CreateGraphicsPipeline(aRenderPass);
}

void AM_SimpleGPUParticleSystem::Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, const TempBuffer* anSSBO)
{
	myGraphicsPipeline.BindGraphics(aCommandBuffer);
	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myGraphicsPipeline.GetPipelineLayout(), 0, 1, &aDescriptorSet, 0, nullptr);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(aCommandBuffer, 0, 1, &anSSBO->myBuffer, offsets);
	static constexpr int PARTICLE_COUNT = 2000;
	vkCmdDraw(aCommandBuffer, PARTICLE_COUNT, 1, 0, 0);
}

void AM_SimpleGPUParticleSystem::CreateGraphicsPipeline(VkRenderPass aRenderPass)
{
	AM_PipelineUtils::GraphicsInitializer graphicsInitializer{};
	AM_PipelineUtils::GetDefaultStates(graphicsInitializer);
	AM_PipelineUtils::EnableMultiSampleState(graphicsInitializer, myVkContext.maxMSAASamples);

	VkPipelineVertexInputStateCreateInfo& vertexInputInfo = graphicsInitializer.vertexInputState;
	auto bindingDescription = Particle::GetBindingDescription();
	auto attributeDescriptions = Particle::GetAttributeDescriptions();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo& inputAssembly = graphicsInitializer.inputAssemblyState;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

	VkPipelineRasterizationStateCreateInfo& rasterizer = graphicsInitializer.rasterizationState;
	rasterizer.polygonMode = VK_POLYGON_MODE_POINT;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	AM_PipelineUtils::FillPiplineCreateInfo(pipelineInfo, graphicsInitializer);
	pipelineInfo.renderPass = aRenderPass;

	AM_VkDescriptorSetLayoutBuilder builder2;
	builder2.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	builder2.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	myGraphicsPipeline.CreatePipeline(
		"../data/shader_bytecode/particle.vert.spv",
		"../data/shader_bytecode/particle.frag.spv",
		builder2,
		0,
		0,
		pipelineInfo);
}
