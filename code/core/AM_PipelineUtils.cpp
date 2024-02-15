#include "AM_PipelineUtils.h"

void AM_PipelineUtils::GetDefaultStates(AM_PipelineUtils::GraphicsInitializer& outInitializer)
{
	VkPipelineInputAssemblyStateCreateInfo& inputAssembly = outInitializer.inputAssemblyState;
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors are given during drawing
	VkPipelineViewportStateCreateInfo& viewportState = outInitializer.viewportState;
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;

	VkPipelineRasterizationStateCreateInfo& rasterizer = outInitializer.rasterizationState;
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo& multisampling = outInitializer.multisampleState;
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.f; // min fraction for sample shading; closer to one is smoother
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState& colorBlendAttachment = outInitializer.colorBlendAttachmentState;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo& colorBlending = outInitializer.colorBlendState;
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	outInitializer.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	outInitializer.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

	VkPipelineDynamicStateCreateInfo& dynamicState = outInitializer.dynamicState;
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(outInitializer.dynamicStates.size());
	dynamicState.pDynamicStates = outInitializer.dynamicStates.data();

	VkPipelineDepthStencilStateCreateInfo& depthStencil = outInitializer.depthStencilState;
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional
}

void AM_PipelineUtils::FillPiplineCreateInfo(VkGraphicsPipelineCreateInfo& outCreateInfo, const GraphicsInitializer& anInitializer)
{
	outCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	outCreateInfo.pInputAssemblyState = &anInitializer.inputAssemblyState;
	outCreateInfo.pViewportState = &anInitializer.viewportState;
	outCreateInfo.pRasterizationState = &anInitializer.rasterizationState;
	outCreateInfo.pMultisampleState = &anInitializer.multisampleState;
	outCreateInfo.pDepthStencilState = &anInitializer.depthStencilState;
	outCreateInfo.pColorBlendState = &anInitializer.colorBlendState;
	outCreateInfo.pDynamicState = &anInitializer.dynamicState;
	outCreateInfo.pVertexInputState = &anInitializer.vertexInputState;

	// some default values
	outCreateInfo.subpass = 0;
	outCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	outCreateInfo.basePipelineIndex = -1; // Optional
}

void AM_PipelineUtils::SetDefaultComputeCreateInfo(VkComputePipelineCreateInfo& aCreateInfo)
{
	aCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	VkPipelineShaderStageCreateInfo& computeShaderStageInfo = aCreateInfo.stage;
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.pName = "main";
	computeShaderStageInfo.pNext = nullptr;
	computeShaderStageInfo.pSpecializationInfo = nullptr;
	computeShaderStageInfo.flags = 0;
}

void AM_PipelineUtils::EnableAlphaBlendState(GraphicsInitializer& outInitializer)
{
	VkPipelineColorBlendAttachmentState& colorBlendAttachment = outInitializer.colorBlendAttachmentState;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
}

void AM_PipelineUtils::EnableMultiSampleState(GraphicsInitializer& outInitializer, VkSampleCountFlagBits someBits, float aMinSampleShading /*= 0.2f*/)
{
	VkPipelineMultisampleStateCreateInfo& multisampling = outInitializer.multisampleState;
	multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	multisampling.rasterizationSamples = someBits;
	multisampling.minSampleShading = aMinSampleShading; // min fraction for sample shading; closer to one is smoother
}
