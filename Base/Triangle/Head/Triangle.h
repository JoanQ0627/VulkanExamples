// ����Ҫ����������
// prepare() �� render()
// ��������Щ���ڻ������涨����ģ����������Щ�ط�Щ���Ǻͻ����ͻ��
// �Ҹ�������һ�£�ͳһ�Ի���Ϊ׼
// Ȼ�����һЩ�ṹ��Ķ��壬ÿ�����Ӷ���һ�����ȿ���triangle�İ�
// ��һЩûʲô����������ṹ�壬�����Ͷ��壬�ʹպ����Ű���

#pragma once

#include "VulkanExampleBase.h"

class Triangle : public VulkanExampleBase
{
public:
	// - ���ڽṹ�嶨��
	// ��������Щ�������ᱻ������ gltfmodel �� һЩ��װ����VulkanTexture, VulkanBuffer�����棩
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
