#pragma once
#include "AM_VkContext.h"
#include <vector>

namespace AM_PipelineUtils
{
	struct GraphicsInitializer
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		VkPipelineViewportStateCreateInfo viewportState{};
		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		VkPipelineMultisampleStateCreateInfo multisampleState{};
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		VkPipelineDepthStencilStateCreateInfo depthStencilState{};

		VkPipelineVertexInputStateCreateInfo vertexInputState{};


		std::vector<VkDynamicState> dynamicStates;
	};

	void GetDefaultStates(GraphicsInitializer& outInitializer);
	void EnableAlphaBlendState(GraphicsInitializer& outInitializer);
	void EnableMultiSampleState(GraphicsInitializer& outInitializer, VkSampleCountFlagBits someBits, float aMinSampleShading = 0.2f);
	void FillPiplineCreateInfo(VkGraphicsPipelineCreateInfo& outCreateInfo, const GraphicsInitializer& anInitializer);
	void SetDefaultComputeCreateInfo(VkComputePipelineCreateInfo& aCreateInfo);
}