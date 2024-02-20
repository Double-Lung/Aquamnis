#include "AM_VkRenderMethodMesh.h"

#include "AM_Camera.h"
#include "AM_Entity.h"
#include "TempBuffer.h"

AM_VkRenderMethodMesh::AM_VkRenderMethodMesh(
	AM_VkContext& aVkContext, 
	const VkRenderPass aRenderPass, 
	const std::string& aVertexShaderPath,
	const std::string& aFragmentShaderPath,
	uint32_t aBindingDescriptionCount /*= 1*/, 
	uint32_t anAttributeDescriptionCount /*= 1*/, 
	const VkVertexInputBindingDescription* aBindingDescription /*= nullptr*/, 
	const VkVertexInputAttributeDescription* anAttributeDescription /*= nullptr*/) 
	: AM_VkRenderMethod<AM_VkRenderMethodMesh>(
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

void AM_VkRenderMethodMesh::CreatePipeline_Imp(AM_VkDescriptorSetLayoutBuilder& outBuilder, VkPushConstantRange& outRange, AM_PipelineUtils::GraphicsInitializer& /*outPipelineInitializer*/)
{
	outBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	outBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	outRange.offset = 0;
	outRange.size = sizeof(PushConstantData);
	outRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
}

void AM_VkRenderMethodMesh::Render_Imp(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*>& someEntities, const TempBuffer* /*aBuffer*/)
{
	VkCommandBuffer commandBuffer = someInfo.myCommandBuffer;
	myPipeline.BindGraphics(commandBuffer);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 0, 1, &someInfo.myGlobalDescriptorSet, 0, nullptr);
	for (AM_Entity* entity : someEntities)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 1, 1, &entity->GetDescriptorSets()[someInfo.myFrameIndex], 0, nullptr);
		const TempBuffer* vertexBuffer = entity->GetTempVertexBuffer();
		const TempBuffer* indexBuffer = entity->GetTempIndexBuffer();
		VkBuffer vertexBuffers[] = { vertexBuffer->myBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->myBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, entity->GetIndexBufferSize(), 1, 0, 0, 0);
	}
}
