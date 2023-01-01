#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "VkDrawContext.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

class VkDrawContext;

class VkDraw
{
public:
	// Create window and Vulkan instance
	void Engage();
	
private:
	struct Vertex {
		glm::vec3 myPosition;
		glm::vec3 myColor;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
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
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	std::vector<VkExtensionProperties> GetAvailableExtensions();
	std::vector<const char*> GetRequiredExtensions();
	bool CheckExtensionSupport();
	bool CheckValidationLayerSupport();
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void CreateInstance();
	void InitWindow();

	void GetSwapChainImages();
	void CreateSwapChain();

	void CleanupSwapChain();
	void RecreateSwapChain();
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels);
	void CreateImageViews();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateTextureImageView();
	void CreateImage(uint32_t width, uint32_t height, uint32_t aMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void CreateTextureImage();
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t aMipLevels);
	void CreateColorResources();

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentImage);

	VkCommandBuffer BeginSingleTimeCommands(VkCommandPool aCommandPool);
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool aCommandPool, VkQueue aVkQueue);

	void CreateCommandBuffers();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t imageIndex);
	void CreateSyncObjects();
	void DrawFrame();

	void LoadModel();

	void CreateTextureSampler();
	void CreateDepthResources();
	bool HasStencilComponent(VkFormat format);
	void InitVulkan();
	void CreateRenderPass();
	void MainLoop();
	void Cleanup();

#ifdef _DEBUG
	void ShowExtensionSupportStatus();
	void SetupDebugMessenger();
#endif

	static std::vector<char> ReadFile(const std::string& filename);
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	GLFWwindow* myWindow = nullptr;
	VkDebugUtilsMessengerEXT myDebugMessenger = nullptr;
	VkSwapchainKHR mySwapChain = nullptr;
	VkRenderPass myRenderPass = nullptr;
	VkDescriptorSetLayout myDescriptorSetLayout = nullptr;
	VkPipelineLayout myPipelineLayout = nullptr;
	VkPipeline myGraphicsPipeline = nullptr;
	VkCommandPool myCommandPool;
	VkCommandPool myTransferCommandPool;
	VkDescriptorPool myDescriptorPool;
	std::vector<VkDescriptorSet> myDescriptorSets;
	VkBuffer myVertexBuffer = nullptr;
	VkDeviceMemory myVertexBufferMemory = nullptr;
	VkBuffer myIndexBuffer = nullptr;
	VkDeviceMemory myIndexBufferMemory = nullptr;
	VkImage myTextureImage = nullptr;
	VkDeviceMemory myTextureImageMemory = nullptr;
	VkImageView myTextureImageView = nullptr;
	VkSampler myTextureSampler = nullptr;
	VkImage myDepthImage = nullptr;
	VkDeviceMemory myDepthImageMemory = nullptr;
	VkImageView myDepthImageView = nullptr;

	VkImage myColorImage;
	VkDeviceMemory myColorImageMemory;
	VkImageView myColorImageView;

	uint32_t myMipLevels = 0;

	std::vector<VkBuffer> myUniformBuffers;
	std::vector<VkDeviceMemory> myUniformBuffersMemory;
	std::vector<void*> myUniformBuffersMapped;

	std::vector<VkSemaphore> myImageAvailableSemaphores;
	std::vector<VkSemaphore> myRenderFinishedSemaphores;
	std::vector<VkFence> myInFlightFences;

	std::vector<VkImage> mySwapChainImages;
	std::vector<VkImageView> mySwapChainImageViews;
	std::vector<VkFramebuffer> mySwapChainFramebuffers;
	std::vector<VkCommandBuffer> myCommandBuffers;

	uint32_t myCurrentFrame = 0;
	bool myIsFramebufferResized = false;

	std::vector<Vertex> myVertices;
	std::vector<uint32_t> myIndices;

	VkDrawContext myVkContext;
};

