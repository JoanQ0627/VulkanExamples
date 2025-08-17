#pragma once

#include "Camera.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

class VulkanExampleBase
{
public:
	Camera camera;
	std::string title = "VulkanExampleBase";

public:
	VulkanExampleBase();
	virtual ~VulkanExampleBase();

	void Run();
	void InitWindow();
	void InitVulkan();


protected:
	// 同步原语
	struct VulkanSemaphore
	{
		VkSemaphore presentComplete;
		VkSemaphore renderComplete;
	};

	std::vector<VulkanSemaphore> semaphores;
	std::vector<VkFence> waitFences;

	// 通用的VK变量
	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkCommandPool commandPool;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// 和CPU端多个飞行帧相关的变量
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkImage> swapChainImages;

	// 可能暂时只有一个的vk变量

};