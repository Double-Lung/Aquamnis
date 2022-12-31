#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

class VkDrawContext
{
public:
	VkDrawContext();
	~VkDrawContext() = default;

	bool TryGetQueueFamilies(VkPhysicalDevice device, int& transferQueueIdx, int& graphicsQueueIdx, int& presentQueueIdx);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	void ChoosePhysicalDevice();
	void CreateLogicalDevice();
	
	void GetSwapChainCreationInfo();
	void UpdateSwapChainExtent(uint32_t width, uint32_t height);
	void CreateSwapChain(VkSwapchainKHR& swapChain);

	void GetMaxMSAASampleCount();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling& tiling, const VkFormatFeatureFlags& features) const;
	void GetDepthFormat();

	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	VkSurfaceFormatKHR surfaceFormat;
	VkExtent2D swapChainExtent;
	
	const std::vector<const char*> validationLayers;
	const std::vector<const char*> deviceExtensions;
	std::vector<VkSurfaceFormatKHR> surfaceFormats; // optional for now
	std::vector<VkPresentModeKHR> presentModes; // optional for now
	std::vector<const char*> instanceExtensions;

	static VkInstance instance;
	static VkSurfaceKHR surface;
	static VkPhysicalDevice physicalDevice;
	static VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;

	VkPresentModeKHR presentMode;
	VkFormat depthFormat;
	VkSampleCountFlagBits maxMSAASamples;
	uint32_t sawpChainImageCount;
	uint32_t graphicsFamilyIndex;
	uint32_t presentFamilyIndex;
	uint32_t transferFamilyIndex;
};