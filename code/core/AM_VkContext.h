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
#include "AM_VkRenderCoreConstants.h"
#include <vulkan/vulkan.h>

struct AM_VkContext
{
	AM_VkContext();
	~AM_VkContext();

	void Init();
	void GetAvailableInstanceExtensions();
	void GetRequiredInstanceExtensions();
	void GetAvailableInstanceLayers();
	bool TryGetQueueFamilies(VkPhysicalDevice device, int& transferQueueIdx, int& graphicsQueueIdx, int& presentQueueIdx, int& computeQueueIdx);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	void ChoosePhysicalDevice();
	void CreateLogicalDevice();
	void InitSwapChainCreationInfo();

	void FindMaxMSAASampleCount();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling& tiling, const VkFormatFeatureFlags& features) const;
	void GetDepthFormat();

	void CreateCommandPools();

	// --- vk object creation functions ---
	VkSemaphore CreateSemaphore();
	void DestroySemaphore(VkSemaphore aSemaphore) { vkDestroySemaphore(device, aSemaphore, nullptr); }

	VkFence CreateFence();
	void DestroyFence(VkFence aFence){ vkDestroyFence(device, aFence, nullptr); }

	VkPipelineLayout CreatePipelineLayout(const VkPipelineLayoutCreateInfo& aCreateInfo);
	void DestroyPipelineLayout(VkPipelineLayout aLayout){ vkDestroyPipelineLayout(device, aLayout, nullptr); }

	VkFramebuffer CreateFrameBuffer(const VkFramebufferCreateInfo& aCreateInfo);
	void DestroyFrameBuffer(VkFramebuffer aFrameBuffer) { vkDestroyFramebuffer(device, aFrameBuffer, nullptr); }

	VkCommandBuffer AllocateCommandBuffer(const VkCommandBufferAllocateInfo& aCreateInfo);

	VkSampler CreateSampler(const VkSamplerCreateInfo& aCreateInfo);
	void DestroySampler(VkSampler aSampler);

	VkImageView CreateImageView(const VkImageViewCreateInfo& aCreateInfo);
	void DestroyImageView(VkImageView anImageView) { vkDestroyImageView(device, anImageView, nullptr); }

	VkRenderPass CreateRenderPass(const VkRenderPassCreateInfo& aCreateInfo);
	void DestroyRenderPass(VkRenderPass aRenderPass) { vkDestroyRenderPass(device, aRenderPass, nullptr); }

	VkCommandPool CreateCommandPool(const VkCommandPoolCreateInfo& aCreateInfo);
	void DestroyCommandPool(VkCommandPool aCommandPool) { vkDestroyCommandPool(device, aCommandPool, nullptr); }

	VkDescriptorPool CreateDescriptorPool(uint32_t aMaxSetCount, VkDescriptorPoolCreateFlags somePoolFlags, const std::vector<VkDescriptorPoolSize>& somePoolSizes);
	void DestroyDescriptorPool(VkDescriptorPool aPool) { vkDestroyDescriptorPool(device, aPool, nullptr); }
	void ResetDescriptorPool(VkDescriptorPool aPool) { vkResetDescriptorPool(device, aPool, 0); }

	void AllocateDescriptorSets(VkDescriptorPool aPool, std::vector<VkDescriptorSetLayout>& someDescriptorSetLayouts, std::vector<VkDescriptorSet>& outDescriptorSets);
	void WriteToDescriptorSet(VkDescriptorSet aDescriptorSet, std::vector<VkWriteDescriptorSet>& someWrites);
	void FreeDescriptorSets(VkDescriptorPool aPool, std::vector<VkDescriptorSet>& someDescriptorSets)
	{
		vkFreeDescriptorSets(device, aPool, static_cast<uint32_t>(someDescriptorSets.size()), someDescriptorSets.data());
	}

	VkDescriptorSetLayout CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& someBindings);
	void DestroyDescriptorSetLayout(VkDescriptorSetLayout aLayout) { vkDestroyDescriptorSetLayout(device, aLayout, nullptr); }
	// --- vk object creation functions ---

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
	std::vector<VkCommandPool> myCommandPools;
	std::vector<VkCommandPool> myComputeCommandPools;
	VkCommandPool myTransferCommandPool;

	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

#if _DEBUG
	VkDebugUtilsMessengerEXT myDebugMessenger;
#endif

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;
	VkQueue computeQueue;

	VkPresentModeKHR presentMode;
	VkFormat depthFormat;
	VkSampleCountFlagBits maxMSAASamples;
	VkSampleCountFlagBits globalMSAASamples;
	uint32_t swapChainImageCount;
	uint32_t graphicsFamilyIndex;
	uint32_t presentFamilyIndex;
	uint32_t transferFamilyIndex;
	uint32_t computeFamilyIndex;
};