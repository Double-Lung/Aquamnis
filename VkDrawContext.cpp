#include "VkDrawContext.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>
#include "VkDrawConstants.h"

VkInstance VkDrawContext::instance = VK_NULL_HANDLE;
VkSurfaceKHR VkDrawContext::surface = VK_NULL_HANDLE;
VkPhysicalDevice VkDrawContext::physicalDevice = VK_NULL_HANDLE;
VkDevice VkDrawContext::device = VK_NULL_HANDLE;

VkDrawContext::VkDrawContext() 
	: deviceProperties{}
	, deviceFeatures{}
	, surfaceCapabilities{}
	, memoryProperties{}
	, surfaceFormat{}
	, enabledInstanceLayers
    {
#if _DEBUG
		"VK_LAYER_KHRONOS_validation" 
#endif
    }
	, deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME }
	, graphicsQueue(VK_NULL_HANDLE)
	, presentQueue(VK_NULL_HANDLE)
	, transferQueue(VK_NULL_HANDLE)
	, presentMode(VK_PRESENT_MODE_MAX_ENUM_KHR)
	, depthFormat(VK_FORMAT_MAX_ENUM)
	, maxMSAASamples(VK_SAMPLE_COUNT_1_BIT)
	, swapChainImageCount(0)
	, graphicsFamilyIndex(0)
	, presentFamilyIndex(0)
	, transferFamilyIndex(0)
{
	GetAvailableInstanceExtensions();
#if _DEBUG
	GetAvailableInstanceLayers();
#endif
}

bool VkDrawContext::TryGetQueueFamilies(VkPhysicalDevice device, int& transferQueueIdx, int& graphicsQueueIdx, int& presentQueueIdx)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (!(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport))
			continue;

		graphicsQueueIdx = i;
		presentQueueIdx = i;
		break;
	}

	for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT || !(queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
			continue;
		transferQueueIdx = i;
	}
	return (graphicsQueueIdx >= 0) && (presentQueueIdx >= 0) && (transferQueueIdx >= 0);
}


bool VkDrawContext::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

void VkDrawContext::Init()
{
	ChoosePhysicalDevice();
	CreateLogicalDevice();
	InitSwapChainCreationInfo();
	GetMaxMSAASampleCount();
	GetDepthFormat();
}

void VkDrawContext::GetAvailableInstanceExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	availableInstanceExtensions.resize(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data());
}

void VkDrawContext::GetRequiredInstanceExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	requiredInstanceExtensions.reserve(glfwExtensionCount + 1);
	requiredInstanceExtensions.insert(requiredInstanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef _DEBUG
	requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
}

void VkDrawContext::GetAvailableInstanceLayers()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	availableInstanceLayers.resize(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableInstanceLayers.data());
}

void VkDrawContext::ChoosePhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(VkDrawContext::instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(VkDrawContext::instance, &deviceCount, devices.data());

	for (const VkPhysicalDevice availableDevice : devices)
	{
		if (!CheckDeviceExtensionSupport(availableDevice))
			continue;

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(availableDevice, surface, &formatCount, nullptr);
		if (formatCount < 1)
			continue;

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(availableDevice, surface, &presentModeCount, nullptr);
		if (presentModeCount < 1)
			continue;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(availableDevice, &props);
		if (!(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
			continue;

		VkPhysicalDeviceFeatures feats;
		vkGetPhysicalDeviceFeatures(availableDevice, &feats);
		if (!(feats.geometryShader && feats.samplerAnisotropy && feats.sampleRateShading))
			continue;

		int transferQueueIdx = -1, graphicsQueueIdx = -1, presentQueueIdx = -1;
		if (!TryGetQueueFamilies(availableDevice, transferQueueIdx, graphicsQueueIdx, presentQueueIdx))
			continue;

		// We find a suitable device
		surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(availableDevice, surface, &formatCount, surfaceFormats.data());

		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(availableDevice, surface, &presentModeCount, presentModes.data());

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(availableDevice, surface, &surfaceCapabilities);

		vkGetPhysicalDeviceMemoryProperties(availableDevice, &memoryProperties);

		deviceProperties = std::move(props);
		deviceFeatures = std::move(feats);
		transferFamilyIndex = static_cast<uint32_t>(transferQueueIdx);
		graphicsFamilyIndex = static_cast<uint32_t>(graphicsQueueIdx);
		presentFamilyIndex = static_cast<uint32_t>(presentQueueIdx);
		physicalDevice = availableDevice;
		return;
	}
	throw std::runtime_error("failed to find a suitable GPU!");
}

void VkDrawContext::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	std::unordered_set<uint32_t> uniqueQueueFamilies = { graphicsFamilyIndex, presentFamilyIndex, transferFamilyIndex };
	float queuePriority = 1.0f;
	for ( const uint32_t& queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(enabledInstanceLayers.size());
	createInfo.ppEnabledLayerNames = enabledInstanceLayers.data();

	if ( vkCreateDevice(VkDrawContext::physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS )
		throw std::runtime_error("failed to create logical device!");

	vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentFamilyIndex, 0, &presentQueue);
	vkGetDeviceQueue(device, transferFamilyIndex, 0, &transferQueue);
}

void VkDrawContext::InitSwapChainCreationInfo()
{
	surfaceFormat = surfaceFormats[0];
	for (const VkSurfaceFormatKHR& availableFormat : surfaceFormats)
	{
		if (availableFormat.format != VK_FORMAT_B8G8R8A8_SRGB || availableFormat.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			continue;
		surfaceFormat = availableFormat;
		break;
	}

	presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const VkPresentModeKHR& availablePresentMode : presentModes)
	{
		if (availablePresentMode != VK_PRESENT_MODE_IMMEDIATE_KHR)
			continue;
		presentMode = availablePresentMode;
	}
	for (const VkPresentModeKHR& availablePresentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			break;

		if (availablePresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
			continue;
		presentMode = availablePresentMode;
	}
	swapChainImageCount = std::clamp(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);   
}

void VkDrawContext::GetMaxMSAASampleCount()
{
	VkSampleCountFlags counts =
		deviceProperties.limits.framebufferColorSampleCounts
		& deviceProperties.limits.framebufferDepthSampleCounts;

	if ( counts & VK_SAMPLE_COUNT_64_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_64_BIT;
	else if ( counts & VK_SAMPLE_COUNT_32_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_32_BIT;
	else if ( counts & VK_SAMPLE_COUNT_16_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_16_BIT;
	else if ( counts & VK_SAMPLE_COUNT_8_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_8_BIT;
	else if ( counts & VK_SAMPLE_COUNT_4_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_4_BIT;
	else if ( counts & VK_SAMPLE_COUNT_2_BIT )
		maxMSAASamples = VK_SAMPLE_COUNT_2_BIT;
	else
		maxMSAASamples = VK_SAMPLE_COUNT_1_BIT;
}

VkFormat VkDrawContext::FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling& tiling, const VkFormatFeatureFlags& features) const
{
	for (const VkFormat& format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && ((props.linearTilingFeatures & features) ^ features) == 0)
			return format;

		if (tiling == VK_IMAGE_TILING_OPTIMAL && ((props.optimalTilingFeatures & features) ^ features) == 0)
			return format;
	}
	throw std::runtime_error("failed to find supported format!");
}

void VkDrawContext::GetDepthFormat()
{
	depthFormat = FindSupportedFormat
	(
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}
