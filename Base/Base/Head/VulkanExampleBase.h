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
	// ͬ��ԭ��
	struct VulkanSemaphore
	{
		VkSemaphore presentComplete;
		VkSemaphore renderComplete;
	};

	std::vector<VulkanSemaphore> semaphores;
	std::vector<VkFence> waitFences;

	// ͨ�õ�VK����
	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkCommandPool commandPool;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// ��CPU�˶������֡��صı���
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkImage> swapChainImages;

	// ������ʱֻ��һ����vk����

};