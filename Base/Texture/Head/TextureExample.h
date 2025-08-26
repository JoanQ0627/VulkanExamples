// ��Ҫ��ѧϰ����load�����������
// ����image, imageview�ȵȵĹ���
// ��texture���ӿ�ʼ����ʼʹ��vks::buffer��װ��

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
	// - ���ڽṹ�嶨��
// ��������Щ�������ᱻ������ gltfmodel �� һЩ��װ����VulkanTexture, VulkanBuffer�����棩
	struct Vertex
	{
		float position[3];
		float uv[2];
		float normal[3];
	};

	struct Texture
	{
		// ������ image imagelayout imageview memory width height miplv
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
	// �����3��1��ȷʵ����
	void createDescriptors();

	void createPipelines();

	void updateUniformBuffers();
	void buildCommandBuffers();
};
