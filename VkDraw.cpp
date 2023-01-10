#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "VkDraw.h"
#include "ApplicationConstants.h"
#include "AM_StagingBuffer.h"
#ifdef _DEBUG
#include "extensionProxy.h"
#endif
#include "VkDrawConstants.h"
#include <algorithm>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <unordered_set>

// TODO: 
// All of the helper functions that submit commands so far have been set up to execute synchronously 
// by waiting for the queue to become idle. For practical applications it is recommended to combine these 
// operations in a single command buffer and execute them asynchronously for higher throughput, especially 
// the transitions and copy in the createTextureImage function. Try to experiment with this by creating a 
// setupCommandBuffer that the helper functions record commands into, and add a flushSetupCommands to execute 
// the commands that have been recorded so far. It's best to do this after the texture mapping works to check 
// if the texture resources are still set up correctly.

void VkDraw::Engage()
{
	InitWindow();
	InitVulkan();
	MainLoop();
	Cleanup();
}

bool VkDraw::CheckExtensionSupport()
{
#ifdef _DEBUG
	std::unordered_set<std::string> requiredExtensionSet(myVkContext.requiredInstanceExtensions.cbegin(), myVkContext.requiredInstanceExtensions.cend());
	std::cout << "Extension Status:\n";
	for (const auto& extension : myVkContext.availableInstanceExtensions)
	{
		std::cout << '\t' << extension.extensionName;
		if (requiredExtensionSet.find(extension.extensionName) != requiredExtensionSet.cend())
			std::cout << ": Yes" << '\n';
		else
			std::cout << ": No" << '\n';
	}
#endif

	std::unordered_set<std::string> availableExtensionSet;
	for (const auto& extension : myVkContext.availableInstanceExtensions)
		availableExtensionSet.insert(extension.extensionName);

	for (const char* extensionName : myVkContext.requiredInstanceExtensions) 
		if (availableExtensionSet.find(extensionName) == availableExtensionSet.cend())
			return false;

	return true;
}

bool VkDraw::CheckInstanceLayerSupport()
{
	std::unordered_set<std::string> availableLayerSet;
	for (const auto& layerProperties : myVkContext.availableInstanceLayers)
		availableLayerSet.insert(layerProperties.layerName);

	for (const char* layerName : myVkContext.enabledInstanceLayers) 
		if (availableLayerSet.find(layerName) == availableLayerSet.cend())
			return false;
	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkDraw::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void VkDraw::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

#ifdef _DEBUG
void VkDraw::SetupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(VkDrawContext::instance, &createInfo, nullptr, &myDebugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}
#endif

std::vector<char> VkDraw::ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

void VkDraw::FramebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/)
{
	VkDraw* app = reinterpret_cast<VkDraw*>(glfwGetWindowUserPointer(window));
	app->myIsFramebufferResized = true;
}

void VkDraw::CreateInstance()
{
	if (!CheckExtensionSupport())
		throw std::runtime_error("extensions requested by GLFW, but not available!");

	if (!CheckInstanceLayerSupport())
		throw std::runtime_error("layers requested by application, but not available!");

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = ApplicationConstants::WINDOWNAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = ApplicationConstants::WINDOWNAME;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(myVkContext.requiredInstanceExtensions.size());
	createInfo.ppEnabledExtensionNames = myVkContext.requiredInstanceExtensions.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(myVkContext.enabledInstanceLayers.size());
	createInfo.ppEnabledLayerNames = myVkContext.enabledInstanceLayers.data();

#ifdef _DEBUG
	std::vector<VkValidationFeatureEnableEXT> enables =
	{ VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };

	VkValidationFeaturesEXT features = {};
	features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	features.enabledValidationFeatureCount = static_cast<uint32_t>(enables.size());
	features.pEnabledValidationFeatures = enables.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	PopulateDebugMessengerCreateInfo(debugCreateInfo);
	debugCreateInfo.pNext = &features;
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &VkDrawContext::instance) != VK_SUCCESS)
		throw std::runtime_error("failed to create Vulkan instance!");

	if (glfwCreateWindowSurface(VkDrawContext::instance, myWindow, nullptr, &VkDrawContext::surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");
}

void VkDraw::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	myWindow = glfwCreateWindow(ApplicationConstants::MIN_WIDTH, 
		ApplicationConstants::MIN_HEIGHT, 
		ApplicationConstants::WINDOWNAME, 
		nullptr, nullptr);
	glfwSetWindowUserPointer(myWindow, this);
	glfwSetFramebufferSizeCallback(myWindow, FramebufferResizeCallback);
	glfwSetWindowSizeLimits
	(
		myWindow, 
		ApplicationConstants::MIN_WIDTH, 
		ApplicationConstants::MIN_HEIGHT, 
		ApplicationConstants::MAX_WIDTH, 
		ApplicationConstants::MAX_HEIGHT
	);
	myVkContext.GetRequiredInstanceExtensions();
}

void VkDraw::GetSwapChainImages()
{
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(VkDrawContext::device, mySwapChain, &imageCount, nullptr);
	mySwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(VkDrawContext::device, mySwapChain, &imageCount, mySwapChainImages.data());
}

void VkDraw::CreateSwapChain()
{
	int width, height;
	glfwGetFramebufferSize(myWindow, &width, &height);
	myVkContext.UpdateSwapChainExtent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	myVkContext.CreateSwapChain(mySwapChain);
	GetSwapChainImages();
}

void VkDraw::CleanupSwapChain()
{
	vkDestroyImageView(VkDrawContext::device, myColorImageView, nullptr);
	vkDestroyImage(VkDrawContext::device, myColorImage, nullptr);
	vkFreeMemory(VkDrawContext::device, myColorImageMemory, nullptr);
	vkDestroyImageView(VkDrawContext::device, myDepthImageView, nullptr);
	vkDestroyImage(VkDrawContext::device, myDepthImage, nullptr);
	vkFreeMemory(VkDrawContext::device, myDepthImageMemory, nullptr);

	for (size_t i = 0; i < mySwapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(VkDrawContext::device, mySwapChainFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < mySwapChainImageViews.size(); i++) {
		vkDestroyImageView(VkDrawContext::device, mySwapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(VkDrawContext::device, mySwapChain, nullptr);
}

void VkDraw::RecreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(myWindow, &width, &height);
	while (width == 0 || height == 0) 
	{
		glfwGetFramebufferSize(myWindow, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(VkDrawContext::device);

	CleanupSwapChain();

	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();
}

VkImageView VkDraw::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = aMipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(VkDrawContext::device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture image view!");

	return imageView;
}

void VkDraw::CreateSwapChainImageViews()
{
	mySwapChainImageViews.resize(mySwapChainImages.size());

	for (uint32_t i = 0; i < mySwapChainImages.size(); i++)
		mySwapChainImageViews[i] = CreateImageView(mySwapChainImages[i], myVkContext.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VkDraw::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uniformBufferObjectLayoutBinding{};
	uniformBufferObjectLayoutBinding.binding = 0;
	uniformBufferObjectLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferObjectLayoutBinding.descriptorCount = 1;
	uniformBufferObjectLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uniformBufferObjectLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uniformBufferObjectLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(VkDrawContext::device, &layoutInfo, nullptr, &myDescriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");
}

void VkDraw::CreateGraphicsPipeline()
{
	auto vertShaderCode = ReadFile("shaders/vert.spv");
	auto fragShaderCode = ReadFile("shaders/frag.spv");
#ifdef _DEBUG
	std::cout << "vert file size: " << vertShaderCode.size() << '\n';
	std::cout << "frag file size: " << fragShaderCode.size() << '\n';
#endif
	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();

	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors are given during drawing
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
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

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	multisampling.rasterizationSamples = myVkContext.maxMSAASamples;
	multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	std::array<VkDynamicState, 2> dynamicStates = 
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &myDescriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(VkDrawContext::device, &pipelineLayoutInfo, nullptr, &myPipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
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

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = myPipelineLayout;
	pipelineInfo.renderPass = myRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(VkDrawContext::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &myGraphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(VkDrawContext::device, fragShaderModule, nullptr);
	vkDestroyShaderModule(VkDrawContext::device, vertShaderModule, nullptr);
}

VkShaderModule VkDraw::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(VkDrawContext::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	return shaderModule;
}

void VkDraw::CreateFramebuffers()
{
	mySwapChainFramebuffers.resize(mySwapChainImageViews.size());

	for (size_t i = 0; i != mySwapChainImageViews.size(); ++i)
	{
		std::array<VkImageView, 3> attachments = { myColorImageView, myDepthImageView ,mySwapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = myRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = myVkContext.swapChainExtent.width;
		framebufferInfo.height = myVkContext.swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(VkDrawContext::device, &framebufferInfo, nullptr, &mySwapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}
}

void VkDraw::CreateCommandPools()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = myVkContext.graphicsFamilyIndex;

	myCommandPools.resize(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);
	for (VkCommandPool& commandPool : myCommandPools)
		if (vkCreateCommandPool(VkDrawContext::device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create command pool!");

	VkCommandPoolCreateInfo transferPoolInfo{};
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	transferPoolInfo.queueFamilyIndex = myVkContext.transferFamilyIndex;

	if (vkCreateCommandPool(VkDrawContext::device, &transferPoolInfo, nullptr, &myTransferCommandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create transfer command pool!");
}

void VkDraw::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(VkDrawContext::device, &poolInfo, nullptr, &myDescriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
}

void VkDraw::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(VkDrawConstants::MAX_FRAMES_IN_FLIGHT, myDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = myDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	myDescriptorSets.resize(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(VkDrawContext::device, &allocInfo, myDescriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets!");

	for (size_t i = 0; i < VkDrawConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = myUniformBuffers[i].myBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject); // or VK_WHOLE_SIZE
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = myTextureImageView;
		imageInfo.sampler = myTextureSampler;
		

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = myDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = nullptr; // Optional
		descriptorWrites[0].pTexelBufferView = nullptr; // Optional

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = myDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(VkDrawContext::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void VkDraw::CreateTextureImageView()
{
	myTextureImageView = CreateImageView(myTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, myMipLevels);
}

void VkDraw::CreateImage(uint32_t width, uint32_t height, uint32_t aMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = aMipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = 0; // Optional

	if (vkCreateImage(VkDrawContext::device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("failed to create image!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(VkDrawContext::device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(VkDrawContext::device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory!");

	vkBindImageMemory(VkDrawContext::device, image, imageMemory, 0);
}

void VkDraw::CreateTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(VkDrawConstants::TEXTURE_PATH, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	myMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	VkDeviceSize imageSize = (uint64_t)texWidth * (uint64_t)texHeight * 4;
	AM_StagingBuffer stagingBuffer(imageSize, myVkContext);

	void* data;
	vkMapMemory(VkDrawContext::device, stagingBuffer.myMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(VkDrawContext::device, stagingBuffer.myMemory);

	stbi_image_free(pixels);

	CreateImage((uint32_t)texWidth, (uint32_t)texHeight, myMipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, myTextureImage, myTextureImageMemory);

	TransitionImageLayout(myTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, myMipLevels);
	CopyBufferToImage(stagingBuffer.myBuffer, myTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	//transitionImageLayout(myTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
	GenerateMipmaps(myTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, myMipLevels);
}

void VkDraw::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(myCommandPools[myCurrentFrame]);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	EndSingleTimeCommands(commandBuffer, myCommandPools[myCurrentFrame], myVkContext.graphicsQueue);
}

void VkDraw::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, AM_SimpleBufferObject& aBufferObject)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkMemoryRequirements memRequirements;
	aBufferObject.Init(memRequirements, bufferInfo);

	uint32_t memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	auto& memoryObject = myMemoryAllocator.Allocate(memoryTypeIndex, memRequirements.size);

	aBufferObject.Bind(&memoryObject);
}

uint32_t VkDraw::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < myVkContext.memoryProperties.memoryTypeCount; ++i)
		if ((typeFilter & (1 << i)) && (myVkContext.memoryProperties.memoryTypes[i].propertyFlags ^ properties) == 0)
			return i;

	throw std::runtime_error("failed to find suitable memory type!");
}

void VkDraw::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(myTransferCommandPool);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer, myTransferCommandPool, myVkContext.transferQueue);
}

void VkDraw::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(myCommandPools[myCurrentFrame]);
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = aMipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (HasStencilComponent(format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		throw std::invalid_argument("unsupported layout transition!");

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(commandBuffer, myCommandPools[myCurrentFrame], myVkContext.graphicsQueue);
}


void VkDraw::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t aMipLevels)
{
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(VkDrawContext::physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("texture image format does not support linear blitting!");

	// It should be noted that it is uncommon in practice to generate the mipmap levels at runtime anyway. 
	// Usually they are pre-generated and stored in the texture file alongside the base level to improve loading speed. 
	// Implementing resizing in software and loading multiple levels from a file is left as an exercise to the reader.

	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(myCommandPools[myCurrentFrame]);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < aMipLevels; i++) 
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = myMipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	EndSingleTimeCommands(commandBuffer, myCommandPools[myCurrentFrame], myVkContext.graphicsQueue);
}

void VkDraw::CreateColorResources()
{
	CreateImage(myVkContext.swapChainExtent.width, myVkContext.swapChainExtent.height, 1, myVkContext.maxMSAASamples, myVkContext.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, myColorImage, myColorImageMemory);
	myColorImageView = CreateImageView(myColorImage, myVkContext.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VkDraw::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(myVertices[0]) * myVertices.size();
	AM_StagingBuffer stagingBuffer(bufferSize, myVkContext);

	void* data;
	vkMapMemory(VkDrawContext::device, stagingBuffer.myMemory, 0, bufferSize, 0, &data);
	memcpy(data, myVertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(VkDrawContext::device, stagingBuffer.myMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, myVertexBuffer);

	CopyBuffer(stagingBuffer.myBuffer, myVertexBuffer.myBuffer, bufferSize);
}

void VkDraw::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(myIndices[0]) * myIndices.size();
	AM_StagingBuffer stagingBuffer(bufferSize, myVkContext);

	void* data;
	vkMapMemory(VkDrawContext::device, stagingBuffer.myMemory, 0, bufferSize, 0, &data);
	memcpy(data, myIndices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(VkDrawContext::device, stagingBuffer.myMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, myIndexBuffer);

	CopyBuffer(stagingBuffer.myBuffer, myIndexBuffer.myBuffer, bufferSize);
}

void VkDraw::CreateUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	myUniformBuffers.resize(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < VkDrawConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, myUniformBuffers[i]);
	}

	vkMapMemory(VkDrawContext::device, myUniformBuffers[0].myMemoryObject->myMemory, myUniformBuffers[0].myMemoryObject->myOffset, bufferSize * VkDrawConstants::MAX_FRAMES_IN_FLIGHT, 0, &myUniformBuffersMapped2);
}

void VkDraw::UpdateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::mat4(1.f);//glm::rotate(glm::mat4(1.f), time * 1.5708f * 0.6667f, glm::vec3(0.f, 1.f, 0.f));
	ubo.view = glm::lookAt(glm::vec3(35.f, 25.f, 35.f), glm::vec3(0.f, 5.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	ubo.projection = glm::perspective(0.7854f, myVkContext.swapChainExtent.width / (float)myVkContext.swapChainExtent.height, 0.1f, 100.f);

	memcpy(myUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

VkCommandBuffer VkDraw::BeginSingleTimeCommands(VkCommandPool aCommandPool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = aCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(VkDrawContext::device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VkDraw::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool aCommandPool, VkQueue aVkQueue)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(aVkQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(aVkQueue);

	vkFreeCommandBuffers(VkDrawContext::device, aCommandPool, 1, &commandBuffer);
}

void VkDraw::CreateReusableCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	myCommandBuffers.resize(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < VkDrawConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		allocInfo.commandPool = myCommandPools[i];
		if (vkAllocateCommandBuffers(VkDrawContext::device, &allocInfo, &myCommandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate command buffer!");
	}
}

void VkDraw::RecordCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer!");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = myRenderPass;
	renderPassInfo.framebuffer = mySwapChainFramebuffers[imageIndex];

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = myVkContext.swapChainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myGraphicsPipeline);

	VkBuffer vertexBuffers[] = { myVertexBuffer.myBuffer };
	VkDeviceSize offsets[] = { myVertexBuffer.myMemoryObject->myOffset };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, myIndexBuffer.myBuffer, myIndexBuffer.myMemoryObject->myOffset, VK_INDEX_TYPE_UINT32);

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = static_cast<float>(myVkContext.swapChainExtent.height);
	viewport.width = static_cast<float>(myVkContext.swapChainExtent.width);
	viewport.height = -static_cast<float>(myVkContext.swapChainExtent.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = myVkContext.swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipelineLayout, 0, 1, &myDescriptorSets[myCurrentFrame], 0, nullptr);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(myIndices.size()), 1, 0, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer!");
}

void VkDraw::CreateSyncObjects()
{
	myInFlightFences.resize(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);
	myImageAvailableSemaphores.resize(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);
	myRenderFinishedSemaphores.resize(VkDrawConstants::MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i != VkDrawConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(VkDrawContext::device, &semaphoreInfo, nullptr, &myImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(VkDrawContext::device, &semaphoreInfo, nullptr, &myRenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(VkDrawContext::device, &fenceInfo, nullptr, &myInFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create semaphores!");
	}
}

void VkDraw::DrawFrame()
{
	vkWaitForFences(VkDrawContext::device, 1, &myInFlightFences[myCurrentFrame], VK_TRUE, UINT64_MAX);
	
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(VkDrawContext::device, mySwapChain, UINT64_MAX, myImageAvailableSemaphores[myCurrentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		RecreateSwapChain();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
		throw std::runtime_error("failed to acquire swap chain image!");

	vkResetFences(VkDrawContext::device, 1, &myInFlightFences[myCurrentFrame]);

	UpdateUniformBuffer(myCurrentFrame);

	vkResetCommandPool(VkDrawContext::device, myCommandPools[myCurrentFrame], VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	RecordCommandBuffer(myCommandBuffers[myCurrentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	std::array<VkSemaphore, 1> waitSemaphores = { myImageAvailableSemaphores[myCurrentFrame] };
	std::array<VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &myCommandBuffers[myCurrentFrame];

	std::array<VkSemaphore, 1> signalSemaphores = { myRenderFinishedSemaphores[myCurrentFrame] };
	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	if (vkQueueSubmit(myVkContext.graphicsQueue, 1, &submitInfo, myInFlightFences[myCurrentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer!");
	
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores.data();

	std::array<VkSwapchainKHR, 1> swapChains = { mySwapChain };
	presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
	presentInfo.pSwapchains = swapChains.data();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;
	result = vkQueuePresentKHR(myVkContext.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || myIsFramebufferResized) 
	{
		myIsFramebufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) 
		throw std::runtime_error("failed to present swap chain image!");

	myCurrentFrame = (myCurrentFrame + 1) % VkDrawConstants::MAX_FRAMES_IN_FLIGHT;
}

void VkDraw::LoadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, VkDrawConstants::MODEL_PATH))
		throw std::runtime_error(warn + err);

	for (const tinyobj::shape_t& shape : shapes) 
	{
		for (const tinyobj::index_t& index : shape.mesh.indices) 
		{
			Vertex vertex{};
			vertex.myPosition = 
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			vertex.texCoord = 
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.myColor = { 1.0f, 1.0f, 1.0f };
			myVertices.push_back(vertex);
			myIndices.push_back(static_cast<uint32_t>(myIndices.size()));
		}
	}
}

void VkDraw::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = myVkContext.deviceProperties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = static_cast<float>(myMipLevels); // or VK_LOD_CLAMP_NONE

	if (vkCreateSampler(VkDrawContext::device, &samplerInfo, nullptr, &myTextureSampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");
}

void VkDraw::CreateDepthResources()
{
	CreateImage(myVkContext.swapChainExtent.width, myVkContext.swapChainExtent.height, 1, myVkContext.maxMSAASamples, myVkContext.depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, myDepthImage, myDepthImageMemory);
	myDepthImageView = CreateImageView(myDepthImage, myVkContext.depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	TransitionImageLayout(myDepthImage, myVkContext.depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

bool VkDraw::HasStencilComponent(VkFormat format)
{
	return !((format ^ VK_FORMAT_D32_SFLOAT_S8_UINT) && (format ^ VK_FORMAT_D24_UNORM_S8_UINT));
}

void VkDraw::InitVulkan()
{
	CreateInstance();
#ifdef _DEBUG
	SetupDebugMessenger();
#endif
	
	myVkContext.Init();
	myMemoryAllocator.Init(myVkContext.memoryProperties.memoryTypeCount);

	// swap chain
	CreateSwapChain();
	CreateSwapChainImageViews();

	CreateCommandPools();
	CreateDescriptorPool();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateColorResources();
	CreateDepthResources();
	CreateUniformBuffers();
	
	// load textures
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();

	// load 3d models
	LoadModel();
	CreateVertexBuffer();
	CreateIndexBuffer();

	CreateReusableCommandBuffers();
	CreateFramebuffers();
	CreateGraphicsPipeline();
	CreateDescriptorSets();
	CreateSyncObjects();
}

void VkDraw::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = myVkContext.surfaceFormat.format;
	colorAttachment.samples = myVkContext.maxMSAASamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = myVkContext.depthFormat;
	depthAttachment.samples = myVkContext.maxMSAASamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = myVkContext.surfaceFormat.format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(VkDrawContext::device, &renderPassInfo, nullptr, &myRenderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

void VkDraw::MainLoop()
{
	while (!glfwWindowShouldClose(myWindow)) 
	{
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(VkDrawContext::device);
}

void VkDraw::Cleanup()
{
	CleanupSwapChain();

	vkDestroySampler(VkDrawContext::device, myTextureSampler, nullptr);
	vkDestroyImageView(VkDrawContext::device, myTextureImageView, nullptr);
	vkDestroyImage(VkDrawContext::device, myTextureImage, nullptr);
	vkFreeMemory(VkDrawContext::device, myTextureImageMemory, nullptr);

	myVertexBuffer.Release();
	myIndexBuffer.Release();

	vkDestroyPipeline(VkDrawContext::device, myGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(VkDrawContext::device, myPipelineLayout, nullptr);

	for (size_t i = 0; i < VkDrawConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		myUniformBuffers[i].Release();
	}

	myMemoryAllocator.DeleteMemoryBlocks();
	vkDestroyDescriptorPool(VkDrawContext::device, myDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(VkDrawContext::device, myDescriptorSetLayout, nullptr);
	vkDestroyRenderPass(VkDrawContext::device, myRenderPass, nullptr);

	for (size_t i = 0; i != VkDrawConstants::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(VkDrawContext::device, myImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(VkDrawContext::device, myRenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(VkDrawContext::device, myInFlightFences[i], nullptr);
	}

	for (VkCommandPool& commandPool : myCommandPools)
		vkDestroyCommandPool(VkDrawContext::device, commandPool, nullptr);

	vkDestroyCommandPool(VkDrawContext::device, myTransferCommandPool, nullptr);
	vkDestroyDevice(VkDrawContext::device, nullptr);

#ifdef _DEBUG
	DestroyDebugUtilsMessengerEXT(VkDrawContext::instance, myDebugMessenger, nullptr);
#endif

	vkDestroySurfaceKHR(VkDrawContext::instance, VkDrawContext::surface, nullptr);
	vkDestroyInstance(VkDrawContext::instance, nullptr);
	glfwDestroyWindow(myWindow);
	glfwTerminate();
}

VkVertexInputBindingDescription VkDraw::Vertex::GetBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VkDraw::Vertex::GetAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, myPosition);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, myColor);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

