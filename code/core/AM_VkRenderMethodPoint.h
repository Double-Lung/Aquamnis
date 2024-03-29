#pragma once
#include "AM_VkRenderMethod.h"
#include <glm/glm.hpp>

class AM_Camera;
class AM_Entity;
struct TempBuffer;
class AM_VkRenderMethodPoint : public AM_VkRenderMethod<AM_VkRenderMethodPoint>
{
public:
	AM_VkRenderMethodPoint(
		AM_VkContext& aVkContext,
		const VkRenderPass aRenderPass,
		const std::string& aVertexShaderPath,
		const std::string& aFragmentShaderPath,
		VkDescriptorSetLayout aGlobalLayout,
		uint32_t aBindingDescriptionCount = 1,
		uint32_t anAttributeDescriptionCount = 1,
		const VkVertexInputBindingDescription* aBindingDescription = nullptr,
		const VkVertexInputAttributeDescription* anAttributeDescription = nullptr);

	AM_VkRenderMethodPoint(const AM_VkRenderMethodPoint&) = delete;
	AM_VkRenderMethodPoint& operator=(const AM_VkRenderMethodPoint&) = delete;

	void CreatePipeline_Imp(AM_VkDescriptorSetLayoutBuilder& outBuilder, VkPushConstantRange& outRange, AM_PipelineUtils::GraphicsInitializer& outPipelineInitializer);
	void Render_Imp(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*>& someEntities, const TempBuffer* aBuffer);
};