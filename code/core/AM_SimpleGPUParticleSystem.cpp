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

AM_SimpleGPUParticleSystem::AM_SimpleGPUParticleSystem(
	AM_VkContext& aVkContext,
	const VkRenderPass aRenderPass,
	const std::string& aVertexShaderPath,
	const std::string& aFragmentShaderPath,
	uint32_t aBindingDescriptionCount,
	uint32_t anAttributeDescriptionCount,
	const VkVertexInputBindingDescription* aBindingDescription,
	const VkVertexInputAttributeDescription* anAttributeDescription)
	: myVkContext{ aVkContext }
	, myGraphicsPipeline{aVkContext}
{
	AM_PipelineUtils::GraphicsInitializer graphicsInitializer{};
	AM_PipelineUtils::GetDefaultStates(graphicsInitializer);
	if (VK_SAMPLE_COUNT_1_BIT < myVkContext.globalMSAASamples)
		AM_PipelineUtils::EnableMultiSampleState(graphicsInitializer, myVkContext.globalMSAASamples);

	VkPipelineVertexInputStateCreateInfo& vertexInputInfo = graphicsInitializer.vertexInputState;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = aBindingDescription ? aBindingDescriptionCount : 0;
	vertexInputInfo.pVertexBindingDescriptions = aBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = anAttributeDescription ? anAttributeDescriptionCount : 0;
	vertexInputInfo.pVertexAttributeDescriptions = anAttributeDescription;


	VkPipelineInputAssemblyStateCreateInfo& inputAssembly = graphicsInitializer.inputAssemblyState;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

	VkPipelineRasterizationStateCreateInfo& rasterizer = graphicsInitializer.rasterizationState;
	rasterizer.polygonMode = VK_POLYGON_MODE_POINT;

	AM_VkDescriptorSetLayoutBuilder builder;
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);


	VkGraphicsPipelineCreateInfo pipelineInfo{};
	AM_PipelineUtils::FillPiplineCreateInfo(pipelineInfo, graphicsInitializer);
	pipelineInfo.renderPass = aRenderPass;
	myGraphicsPipeline.CreatePipeline(aVertexShaderPath, aFragmentShaderPath, builder, pipelineInfo);
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

