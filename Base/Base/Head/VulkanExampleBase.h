#pragma once

// ͷ�ļ�˳�� ƽ̨��C\C++���������⡢�Լ�д��

#ifdef _WIN32
#include <windows.h>
#include <ShellScalingAPI.h>
#endif

#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <array>

#include <vulkan/vulkan.h>


#include "VulkanTools.h"
#include "VulkanDebug.h"
#include "VulkanUIOverlay.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "Camera.hpp"
#include "CommandLineParser.hpp"
#include "benchmark.hpp"
#include "keycodes.hpp"


constexpr uint32_t c_maxConcurrentFrames{ 2 };// ��ʹ��3�����ʱ�򣬺���ȷʵ����2֡����õ�ѡ��

// ����Ų�˳�� public -> private -> protected,  ���� -> ����
class VulkanExampleBase
{
public: // ��������
	
	// ���ڽṹ�嶨��
	struct Settings {
		bool validation = false;
		bool fullscreen = false;
		bool vsync = false;
		bool overlay = true;
	} settings;

	struct {
		glm::vec2 axisLeft = glm::vec2(0.0f);
		glm::vec2 axisRight = glm::vec2(0.0f);
	} gamePadState;

	struct {
		struct {
			bool left = false;
			bool right = false;
			bool middle = false;
		} buttons;
		glm::vec2 position;
	} mouseState;

	struct {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
	} depthStencil{};

	// һЩûʲô�õı���(��������Ⱦ)
	std::string title = "Vulkan Example";
	std::string name = "vulkanExample";
	uint32_t apiVersion = VK_API_VERSION_1_0;

	// tools��ر���
	vks::UIOverlay ui;
	CommandLineParser commandLineParser;
	vks::Benchmark benchmark;
	static std::vector<const char*> args;
	
	// timer���
	float frameTimer = 1.0f;
	float timer = 0.0f;
	float timerSpeed = 0.25f;
	bool paused = false;

	// һЩ����Ϊ�Ƚ���Ҫ�ı���
	vks::VulkanDevice* vulkanDevice{};
	Camera camera;
	VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };
	bool prepared{ false };
	bool resized{ false };
	uint32_t width{ 1280 };
	uint32_t height{ 720 };

	// win32�������
#ifdef _WIN32
	HWND window;
	HINSTANCE windowInstance;
#endif


public: // �ⲿ����
	VulkanExampleBase();
	virtual ~VulkanExampleBase();

	// winmain����
	bool initVulkan();
	virtual void prepare();
	void renderLoop();

	// win32 �������
#ifdef _WIN32
	void setupConsole(std::string title);
	void setupDPIAwareness();
	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif // _WIN32


	// һЩ�麯��
	virtual void keyPressed(uint32_t);
	virtual void mouseMoved(double x, double y, bool& handled);
	virtual void windowResized();
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
#if defined(_WIN32)
	virtual void OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

	virtual VkResult createInstance();
	virtual void render() = 0;  // �ǳ���Ҫ ������ʵ��
	virtual void setupDepthStencil();
	virtual void setupFrameBuffer();
	virtual void setupRenderPass();
	virtual void getEnabledFeatures();
	virtual void getEnabledExtensions();

	// һЩ����ʵ�ּ��ɵĹ�������
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	void windowResize();
	void drawUI(const VkCommandBuffer commandBuffer);

	void prepareFrame(bool waitForFence = true);
	void submitFrame(bool skipQueueSubmit = false);


private: // ����˽�б���
	bool resizing{ false };
	uint32_t destWidth{ 0 };
	uint32_t destHeight{ 0 };
	std::string shaderDir{ "hlsl" };


private: // ����ʵ�ֺ���
	std::string getWindowTitle() const;

	void handleMouseMove(int32_t x, int32_t y);
	void nextFrame();
	void updateOverlay();
	void createPipelineCache(); // todo skip
	//void createCommandPool();
	void createSynchronizationPrimitives();
	void createSurface();
	void createSwapChain();
	void createCommandBuffers();
	void prepareOverlay();

protected: // �������๲�б���

	// properties, features, extensions
	std::vector<std::string> supportedInstanceExtensions;
	VkPhysicalDeviceProperties deviceProperties{};
	VkPhysicalDeviceFeatures deviceFeatures{};
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};

	VkPhysicalDeviceFeatures enabledFeatures{};
	std::vector<const char*> enabledDeviceExtensions;
	std::vector<const char*> enabledInstanceExtensions;
	std::vector<VkLayerSettingEXT> enabledLayerSettings;


	// ͨ�õ�vk����(�����Լ��İ�װ��)
	VkInstance instance{ VK_NULL_HANDLE };
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	VkDevice device{ VK_NULL_HANDLE };
	// VkCommandPool commandPool{ VK_NULL_HANDLE }; // vulkanDevice��װ������Ҳ��һ��commandPool ��������Ҳ���õ�vulkanDevice��commandPool �����������Ҫ��
	VkQueue graphicsQueue{ VK_NULL_HANDLE };
	VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	VulkanSwapChain swapChain;
	VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	std::vector<VkShaderModule> shaderModules;


	// ��CPU�˶��flight֡ �Լ� swapChain���� ��صı���
	uint32_t currentImageIndex{};
	uint32_t currentBuffer{};
	// flight
	std::array<VkCommandBuffer, c_maxConcurrentFrames> drawCmdBuffers;
	std::array<VkFence, c_maxConcurrentFrames> waitFences;
	std::array<VkSemaphore, c_maxConcurrentFrames> presentCompleteSemaphores;
	// swapchainnImageCount
	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkSemaphore> renderCompleteSemaphores;


	// profile��ر���
	uint32_t frameCounter{ 0 };
	uint32_t lastFPS{ 0 };
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp, tPrevEnd;

	// ����(�Լ���ʱ��֪����ô����Ķ���)
	bool requiresStencil{ false };
	void* deviceCreatepNextChain{ nullptr };



protected: // ��������ɼ�������д����
	std::string getShadersPath() const;

	


};