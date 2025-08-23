#pragma once

// 头文件顺序 平台、C\C++、第三方库、自己写的

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


constexpr uint32_t c_maxConcurrentFrames{ 2 };// 当使用3缓冲的时候，好像确实飞行2帧是最好的选择

// 类的排布顺序： public -> private -> protected,  变量 -> 方法
class VulkanExampleBase
{
public: // 公开变量
	
	// 类内结构体定义
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

	// 一些没什么用的变量(对于主渲染)
	std::string title = "Vulkan Example";
	std::string name = "vulkanExample";
	uint32_t apiVersion = VK_API_VERSION_1_0;

	// tools相关变量
	vks::UIOverlay ui;
	CommandLineParser commandLineParser;
	vks::Benchmark benchmark;
	static std::vector<const char*> args;
	
	// timer相关
	float frameTimer = 1.0f;
	float timer = 0.0f;
	float timerSpeed = 0.25f;
	bool paused = false;

	// 一些我认为比较重要的变量
	vks::VulkanDevice* vulkanDevice{};
	Camera camera;
	VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };
	bool prepared{ false };
	bool resized{ false };
	uint32_t width{ 1280 };
	uint32_t height{ 720 };

	// win32窗口相关
#ifdef _WIN32
	HWND window;
	HINSTANCE windowInstance;
#endif


public: // 外部函数
	VulkanExampleBase();
	virtual ~VulkanExampleBase();

	// winmain调用
	bool initVulkan();
	virtual void prepare();
	void renderLoop();

	// win32 窗口相关
#ifdef _WIN32
	void setupConsole(std::string title);
	void setupDPIAwareness();
	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif // _WIN32


	// 一些虚函数
	virtual void keyPressed(uint32_t);
	virtual void mouseMoved(double x, double y, bool& handled);
	virtual void windowResized();
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
#if defined(_WIN32)
	virtual void OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

	virtual VkResult createInstance();
	virtual void render() = 0;  // 非常重要 纯子类实现
	virtual void setupDepthStencil();
	virtual void setupFrameBuffer();
	virtual void setupRenderPass();
	virtual void getEnabledFeatures();
	virtual void getEnabledExtensions();

	// 一些基类实现即可的公开函数
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	void windowResize();
	void drawUI(const VkCommandBuffer commandBuffer);

	void prepareFrame(bool waitForFence = true);
	void submitFrame(bool skipQueueSubmit = false);


private: // 基类私有变量
	bool resizing{ false };
	uint32_t destWidth{ 0 };
	uint32_t destHeight{ 0 };
	std::string shaderDir{ "hlsl" };


private: // 基类实现函数
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

protected: // 基类子类共有变量

	// properties, features, extensions
	std::vector<std::string> supportedInstanceExtensions;
	VkPhysicalDeviceProperties deviceProperties{};
	VkPhysicalDeviceFeatures deviceFeatures{};
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};

	VkPhysicalDeviceFeatures enabledFeatures{};
	std::vector<const char*> enabledDeviceExtensions;
	std::vector<const char*> enabledInstanceExtensions;
	std::vector<VkLayerSettingEXT> enabledLayerSettings;


	// 通用的vk变量(包含自己的包装类)
	VkInstance instance{ VK_NULL_HANDLE };
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	VkDevice device{ VK_NULL_HANDLE };
	// VkCommandPool commandPool{ VK_NULL_HANDLE }; // vulkanDevice包装类里面也有一个commandPool 后面例子也都用的vulkanDevice的commandPool 所以这个不需要了
	VkQueue graphicsQueue{ VK_NULL_HANDLE };
	VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	VulkanSwapChain swapChain;
	VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	std::vector<VkShaderModule> shaderModules;


	// 和CPU端多个flight帧 以及 swapChain数量 相关的变量
	uint32_t currentImageIndex{};
	uint32_t currentBuffer{};
	// flight
	std::array<VkCommandBuffer, c_maxConcurrentFrames> drawCmdBuffers;
	std::array<VkFence, c_maxConcurrentFrames> waitFences;
	std::array<VkSemaphore, c_maxConcurrentFrames> presentCompleteSemaphores;
	// swapchainnImageCount
	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkSemaphore> renderCompleteSemaphores;


	// profile相关变量
	uint32_t frameCounter{ 0 };
	uint32_t lastFPS{ 0 };
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp, tPrevEnd;

	// 其他(以及暂时不知道怎么分类的东西)
	bool requiresStencil{ false };
	void* deviceCreatepNextChain{ nullptr };



protected: // 基类子类可见或需重写函数
	std::string getShadersPath() const;

	


};