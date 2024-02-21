#include "AM_VkRenderMethodPoint.h"

#include "AM_Camera.h"
#include "AM_Entity.h"
#include "TempBuffer.h"

AM_VkRenderMethodPoint::AM_VkRenderMethodPoint(
	AM_VkContext& aVkContext,
	const VkRenderPass aRenderPass,
	const std::string& aVertexShaderPath,
	const std::string& aFragmentShaderPath,
	VkDescriptorSetLayout aGlobalLayout,
	uint32_t aBindingDescriptionCount /*= 1*/,
	uint32_t anAttributeDescriptionCount /*= 1*/,
	const VkVertexInputBindingDescription* aBindingDescription /*= nullptr*/,
	const VkVertexInputAttributeDescription* anAttributeDescription /*= nullptr*/)
	: AM_VkRenderMethod<AM_VkRenderMethodPoint>(
		aVkContext,
		aRenderPass,
		aVertexShaderPath,
		aFragmentShaderPath,
		aBindingDescriptionCount,
		anAttributeDescriptionCount,
		aBindingDescription,
		anAttributeDescription,
		aGlobalLayout)
{
}

void AM_VkRenderMethodPoint::CreatePipeline_Imp(AM_VkDescriptorSetLayoutBuilder& outBuilder, VkPushConstantRange& /*outRange*/, AM_PipelineUtils::GraphicsInitializer& outPipelineInitializer)
{
	VkPipelineInputAssemblyStateCreateInfo& inputAssembly = outPipelineInitializer.inputAssemblyState;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

	VkPipelineRasterizationStateCreateInfo& rasterizer = outPipelineInitializer.rasterizationState;
	rasterizer.polygonMode = VK_POLYGON_MODE_POINT;

	outBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
}

void AM_VkRenderMethodPoint::Render_Imp(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*>& /*someEntities*/, const TempBuffer* aBuffer)
{
	VkCommandBuffer commandBuffer = someInfo.myCommandBuffer;
	myPipeline.BindGraphics(commandBuffer);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 0, 1, &someInfo.myGlobalDescriptorSet, 0, nullptr);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &aBuffer->myBuffer, offsets);
	static constexpr int PARTICLE_COUNT = 2000;
	vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
}

