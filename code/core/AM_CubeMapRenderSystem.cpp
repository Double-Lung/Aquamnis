#include "AM_CubeMapRenderSystem.h"
#include "AM_Entity.h"
#include "AM_Camera.h"
#include "AM_SimpleTimer.h"
#include "AM_VkDescriptorSetLayoutBuilder.h"
#include "AM_RenderUtils.h"
#include "AM_PipelineUtils.h"
#include "vk_mem_alloc.h"
#include "TempBuffer.h"
#include <array>


AM_CubeMapRenderSystem::AM_CubeMapRenderSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass)
	: myVkContext{aVkContext}
	, myGraphicsPipeline{aVkContext}
{
	CreateGraphicsPipeline(aRenderPass);
}

void AM_CubeMapRenderSystem::RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera&/* aCamera*/)
{
	myGraphicsPipeline.BindGraphics(aCommandBuffer);
	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myGraphicsPipeline.GetPipelineLayout(), 0, 1, &aDescriptorSet, 0, nullptr);
	for (auto& entry : someEntites)
	{
		auto& entity = entry.second;
		if (!entity.GetIsCube())
			continue;
		const TempBuffer* vertexBuffer = entity.GetTempVertexBuffer();
		const TempBuffer* indexBuffer = entity.GetTempIndexBuffer();
		VkBuffer vertexBuffers[] = { vertexBuffer->myBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(aCommandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(aCommandBuffer, indexBuffer->myBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(aCommandBuffer, static_cast<uint32_t>(entity.GetIndices().size()), 1, 0, 0, 0);
	}
}

void AM_CubeMapRenderSystem::CreateGraphicsPipeline(VkRenderPass aRenderPass)
{
	AM_PipelineUtils::GraphicsInitializer graphicsInitializer{};
	AM_PipelineUtils::GetDefaultStates(graphicsInitializer);
	AM_PipelineUtils::EnableMultiSampleState(graphicsInitializer, myVkContext.maxMSAASamples);

	VkPipelineVertexInputStateCreateInfo& vertexInputInfo = graphicsInitializer.vertexInputState;
	auto bindingDescriptions = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescriptions;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineRasterizationStateCreateInfo& rasterizer = graphicsInitializer.rasterizationState;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

	VkPipelineDepthStencilStateCreateInfo& depthStencil = graphicsInitializer.depthStencilState;
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	AM_PipelineUtils::FillPiplineCreateInfo(pipelineInfo, graphicsInitializer);
	pipelineInfo.renderPass = aRenderPass;

	AM_VkDescriptorSetLayoutBuilder builder;
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	myGraphicsPipeline.CreatePipeline(
		"../data/shader_bytecode/skybox.vert.spv",
		"../data/shader_bytecode/skybox.frag.spv",
		builder,
		0,
		0,
		pipelineInfo);
}

