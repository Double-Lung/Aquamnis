#include "AM_VkRenderMethodCubeMap.h"

#include "AM_Camera.h"
#include "AM_Entity.h"
#include "TempBuffer.h"

AM_VkRenderMethodCubeMap::AM_VkRenderMethodCubeMap(
	AM_VkContext& aVkContext,
	const VkRenderPass aRenderPass,
	const std::string& aVertexShaderPath,
	const std::string& aFragmentShaderPath,
	uint32_t aBindingDescriptionCount /*= 1*/,
	uint32_t anAttributeDescriptionCount /*= 1*/,
	const VkVertexInputBindingDescription* aBindingDescription /*= nullptr*/,
	const VkVertexInputAttributeDescription* anAttributeDescription /*= nullptr*/)
	: AM_VkRenderMethod<AM_VkRenderMethodCubeMap>(
		aVkContext,
		aRenderPass,
		aVertexShaderPath,
		aFragmentShaderPath,
		aBindingDescriptionCount,
		anAttributeDescriptionCount,
		aBindingDescription,
		anAttributeDescription)
{
}

void AM_VkRenderMethodCubeMap::CreatePipeline_Imp(AM_VkDescriptorSetLayoutBuilder& outBuilder, VkPushConstantRange& /*outRange*/, AM_PipelineUtils::GraphicsInitializer& outPipelineInitializer)
{
	VkPipelineRasterizationStateCreateInfo& rasterizer = outPipelineInitializer.rasterizationState;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

	VkPipelineDepthStencilStateCreateInfo& depthStencil = outPipelineInitializer.depthStencilState;
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	outBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	outBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
}

void AM_VkRenderMethodCubeMap::Render_Imp(VkCommandBuffer aCommandBuffer, VkDescriptorSet aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& /*aCamera*/)
{
	myPipeline.BindGraphics(aCommandBuffer);
	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 0, 1, &aDescriptorSet, 0, nullptr);
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

