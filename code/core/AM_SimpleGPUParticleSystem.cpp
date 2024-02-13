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
	, myComputePipeline{aVkContext}
	, myPipelineLayout{nullptr}
	, myGraphicsPipeline{aVkContext}
	, myGfxPipelineLayout{nullptr}
{
	myMaxComputeWorkGroupCount = myVkContext.deviceProperties.limits.maxComputeWorkGroupCount;
	myMaxComputeWorkGroupInvocations = myVkContext.deviceProperties.limits.maxComputeWorkGroupInvocations;
	myMaxComputeWorkGroupSize = myVkContext.deviceProperties.limits.maxComputeWorkGroupSize;

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	AM_VkDescriptorSetLayoutBuilder builder1;
	builder1.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	builder1.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	builder1.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	builder1.GetBindings(bindings);
	myDescriptorSetLayout = myVkContext.CreateDescriptorSetLayout(bindings);

	AM_VkDescriptorSetLayoutBuilder builder2;
	builder2.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	builder2.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	builder2.GetBindings(bindings);
	myGfxDescriptorSetLayout = myVkContext.CreateDescriptorSetLayout(bindings);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &myGfxDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	myGfxPipelineLayout = myVkContext.CreatePipelineLayout(pipelineLayoutInfo);

	VkPipelineLayoutCreateInfo computePipelineLayoutInfo{};
	computePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computePipelineLayoutInfo.setLayoutCount = 1; // Optional
	computePipelineLayoutInfo.pSetLayouts = &myDescriptorSetLayout;

	myPipelineLayout = myVkContext.CreatePipelineLayout(computePipelineLayoutInfo);

	CreateComputePipeline();
	CreateGraphicsPipeline(aRenderPass);
}

void AM_SimpleGPUParticleSystem::Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, const TempBuffer* anSSBO)
{
	myGraphicsPipeline.BindGraphics(aCommandBuffer);
	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myGfxPipelineLayout, 0, 1, &aDescriptorSet, 0, nullptr);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(aCommandBuffer, 0, 1, &anSSBO->myBuffer, offsets);
	static constexpr int PARTICLE_COUNT = 2000;
	vkCmdDraw(aCommandBuffer, PARTICLE_COUNT, 1, 0, 0);
}

void AM_SimpleGPUParticleSystem::DispatchWork(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional
	if (vkBeginCommandBuffer(aCommandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording compute command buffer!");

	myComputePipeline.BindCompute(aCommandBuffer);
	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, myPipelineLayout, 0, 1, &aDescriptorSet, 0, nullptr);

	static constexpr int PARTICLE_COUNT = 2000;
	vkCmdDispatch(aCommandBuffer, (PARTICLE_COUNT - 1) / 256 + 1, 1, 1);

	if (vkEndCommandBuffer(aCommandBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record compute command buffer!");
}

void AM_SimpleGPUParticleSystem::CreateComputePipeline()
{
	VkComputePipelineCreateInfo pipelineInfo{};
	AM_PipelineUtils::SetDefaultComputeCreateInfo(pipelineInfo);
	pipelineInfo.layout = myPipelineLayout;

	myComputePipeline.CreatePipeline(
		"../data/shader_bytecode/particle.comp.spv", 
		pipelineInfo);
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
	pipelineInfo.layout = myGfxPipelineLayout;
	pipelineInfo.renderPass = aRenderPass;

	myGraphicsPipeline.CreatePipeline(
		"../data/shader_bytecode/particle.vert.spv",
		"../data/shader_bytecode/particle.frag.spv",
		pipelineInfo);
}
