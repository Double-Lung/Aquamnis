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

void AM_VkRenderMethodCubeMap::Render_Imp(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*>& someEntities, const TempBuffer* /*aBuffer*/)
{
	AM_Entity* skyboxEntity = someEntities[0];
	if (!skyboxEntity->GetIsSkybox())
		return;

	VkCommandBuffer commandBuffer = someInfo.myCommandBuffer;
	myPipeline.BindGraphics(commandBuffer);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 0, 1, &someInfo.myGlobalDescriptorSet, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 1, 1, &skyboxEntity->GetDescriptorSets()[someInfo.myFrameIndex], 0, nullptr);

	const TempBuffer* vertexBuffer = skyboxEntity->GetTempVertexBuffer();
	const TempBuffer* indexBuffer = skyboxEntity->GetTempIndexBuffer();
	VkBuffer vertexBuffers[] = { vertexBuffer->myBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->myBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, skyboxEntity->GetIndexBufferSize(), 1, 0, 0, 0);

}

