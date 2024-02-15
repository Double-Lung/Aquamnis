#include "AM_SimpleRenderSystem.h"
#include "AM_Entity.h"
#include "AM_Camera.h"
#include "AM_SimpleTimer.h"
#include "AM_VkDescriptorSetLayoutBuilder.h"
#include "AM_PipelineUtils.h"
#include "AM_RenderUtils.h"
#include "vk_mem_alloc.h"
#include "TempBuffer.h"
#include <array>

AM_SimpleRenderSystem::AM_SimpleRenderSystem(
	AM_VkContext& aVkContext,
	const VkRenderPass aRenderPass,
	const std::string& aVertexShaderPath,
	const std::string& aFragmentShaderPath,
	uint32_t aBindingDescriptionCount,
	uint32_t anAttributeDescriptionCount,
	const VkVertexInputBindingDescription* aBindingDescription,
	const VkVertexInputAttributeDescription* anAttributeDescription)
	: myVkContext(aVkContext)
	, myGraphicsPipeline(aVkContext)
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

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.renderPass = aRenderPass;

	AM_VkDescriptorSetLayoutBuilder builder;
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	VkPushConstantRange aRange{};
	aRange.offset = 0;
	aRange.size = sizeof(PushConstantData);
	aRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	AM_PipelineUtils::FillPiplineCreateInfo(pipelineInfo, graphicsInitializer);
	myGraphicsPipeline.CreatePipeline(aVertexShaderPath, aFragmentShaderPath, builder, pipelineInfo, &aRange);
}

void AM_SimpleRenderSystem::RenderEntities(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera&/* aCamera*/)
{
	myGraphicsPipeline.BindGraphics(aCommandBuffer);
	PushConstantData push;
	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myGraphicsPipeline.GetPipelineLayout(), 0, 1, &aDescriptorSet, 0, nullptr);
	for (auto& entry : someEntites)
	{
		auto& entity = entry.second;
		if (entity.HasPointLightComponent() || entity.GetIsCube())
			continue;
		const TempBuffer* vertexBuffer = entity.GetTempVertexBuffer();
		const TempBuffer* indexBuffer = entity.GetTempIndexBuffer();
		VkBuffer vertexBuffers[] = { vertexBuffer->myBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(aCommandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(aCommandBuffer, indexBuffer->myBuffer, 0, VK_INDEX_TYPE_UINT32);

		push.normalMat = entity.GetTransformComponent().GetNormalMatrix();
		push.transform = entity.GetTransformComponent().GetMatrix();
		vkCmdPushConstants(aCommandBuffer, myGraphicsPipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &push);
		vkCmdDrawIndexed(aCommandBuffer, static_cast<uint32_t>(entity.GetIndices().size()), 1, 0, 0, 0);
	}
}

