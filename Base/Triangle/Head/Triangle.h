// 最重要的两个函数
// prepare() 和 render()
// 其他的有些是在基类里面定义过的，这个例子有些地方些的是和基类冲突的
// 我给他纠正一下，统一以基类为准
// 然后关于一些结构体的定义，每个例子都不一样，先看看triangle的吧
// 有一些没什么意义的匿名结构体，和类型定义，就凑合用着吧先

#pragma once

#include "VulkanExampleBase.h"

class Triangle : public VulkanExampleBase
{
public:
	// - 类内结构体定义
	// （后面这些基本都会被集成在 gltfmodel 和 一些包装类如VulkanTexture, VulkanBuffer等里面）
	struct Vertex 
	{
		float position[3];
		float color[3];
	};

	struct
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
	} vertices;

	struct
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
		uint32_t count{ 0 };
	} indices;

	struct UniformBuffer
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		uint8_t* mapped{ nullptr };
	};

	struct UniformData
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	};
		
	Triangle();
	~Triangle() override;
	void prepare() override;
	void render() override;
private:

	std::array<UniformBuffer, c_maxConcurrentFrames> uniformBuffers;
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };


	void createVertexBuffer();
	void createUniformBuffers();

	void createDescriptorPool(); 
	void createDescriptorSetLayout();
	void createDescriptorSets();

	void createPipelines();

	void updateUniformBuffers();
	void buildCommandBuffers();
};
