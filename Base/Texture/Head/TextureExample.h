// 主要是学习纹理load，纹理采样器
// 纹理image, imageview等等的管理
// 从texture例子开始，开始使用vks::buffer包装类

#pragma once

#include "VulkanExampleBase.h"

class TextureExample : public VulkanExampleBase
{
public:		
	TextureExample();
	~TextureExample() override;
	void prepare() override;
	void render() override;
private:
	// - 类内结构体定义
// （后面这些基本都会被集成在 gltfmodel 和 一些包装类如VulkanTexture, VulkanBuffer等里面）
	struct Vertex
	{
		float position[3];
		float uv[2];
		float normal[3];
	};

	struct Texture
	{
		// 采样器 image imagelayout imageview memory width height miplv
		VkSampler sampler{ VK_NULL_HANDLE };
		VkImage image{ VK_NULL_HANDLE };
		VkImageLayout imageLayout;
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkImageView imageView{ VK_NULL_HANDLE };
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		uint32_t mipLevel{ 0 };
	} texture;

	struct UniformData
	{
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::vec4 viewPos;
		float lodBias{ .0f };
	}uniformData;

	vks::Buffer vertexBuffer;
	vks::Buffer indexBuffer;
	uint32_t indexCount{ 0 };

	std::array<vks::Buffer, c_maxConcurrentFrames> uniformBuffers;

	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };

	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, c_maxConcurrentFrames> descriptorSets;

	void getEnabledFeatures() override;
	void OnUpdateUIOverlay(vks::UIOverlay* overlay) override;

	void loadAndCreateTexture();
	void destroyTextureImage();

	void createVertexBuffer();
	void createUniformBuffers();

	//void createDescriptorPool(); 
	//void createDescriptorSetLayout();
	//void createDescriptorSets();
	// 上面的3合1，确实可以
	void createDescriptors();

	void createPipelines();

	void updateUniformBuffers();
	void buildCommandBuffers();
};
