#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <algorithm>
#ifdef _DEBUG
#include "extensionProxy.h"
#endif
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include "VkDrawConstants.h"
#include <vulkan/vulkan.h>

class VkDrawContext
{
public:
	VkDrawContext();
	~VkDrawContext();

	void Init();
	void GetAvailableInstanceExtensions();
	void GetRequiredInstanceExtensions();
	void GetAvailableInstanceLayers();
	bool TryGetQueueFamilies(VkPhysicalDevice device, int& transferQueueIdx, int& graphicsQueueIdx, int& presentQueueIdx);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	void ChoosePhysicalDevice();
	void CreateLogicalDevice();
	void InitSwapChainCreationInfo();

	void GetMaxMSAASampleCount();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling& tiling, const VkFormatFeatureFlags& features) const;
	void GetDepthFormat();

#ifdef _DEBUG
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void SetupDebugMessenger();
#endif

	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	VkSurfaceFormatKHR surfaceFormat;

	const std::vector<const char*> deviceExtensions;
	const std::vector<const char*> enabledInstanceLayers;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
	std::vector<const char*> requiredInstanceExtensions;
	std::vector<VkExtensionProperties> availableInstanceExtensions;
	std::vector<VkLayerProperties> availableInstanceLayers;

	static VkInstance instance;
	static VkSurfaceKHR surface;
	static VkPhysicalDevice physicalDevice;
	static VkDevice device;

#if _DEBUG
	VkDebugUtilsMessengerEXT myDebugMessenger;
#endif

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;

	VkPresentModeKHR presentMode;
	VkFormat depthFormat;
	VkSampleCountFlagBits maxMSAASamples;
	uint32_t swapChainImageCount;
	uint32_t graphicsFamilyIndex;
	uint32_t presentFamilyIndex;
	uint32_t transferFamilyIndex;
};